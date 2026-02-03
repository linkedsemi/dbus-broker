/*
 * Broker
 */

#include <c-list.h>
#include <c-stdaux.h>
#include <stdlib.h>
#include "broker/broker.h"
#include "broker/controller.h"
#include "broker/main.h"
#include "bus/bus.h"
#include "catalog/catalog-ids.h"
#include "dbus/connection.h"
#include "dbus/message.h"
#include "util/dispatch.h"
#include "util/error.h"
#include "util/log.h"
#include "util/proc.h"
#include "util/sockopt.h"
#include "util/user.h"

#ifdef __ZEPHYR__
#include <zephyr/kernel.h>
#include <zephyr/posix/poll.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/net/socket.h>
#include <zephyr/posix/sys/socket.h>

/* Global broker pointer for external access */
Broker *g_broker = NULL;
#else
#include <sys/signalfd.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#endif

LOG_MODULE_DECLARE(DBUS_BROKER, LOG_LEVEL_DBG);

static int broker_dispatch_signals(DispatchFile *file) {
        Broker *broker = c_container_of(file, Broker, signals_file);

#ifdef __ZEPHYR__
        // Zephyr 不使用 signals_fd，这个函数不会被调用
        return DISPATCH_E_EXIT;
#else
        struct signalfd_siginfo si;
        ssize_t l;

        c_assert(dispatch_file_events(file) == EPOLLIN);

        l = read(broker->signals_fd, &si, sizeof(si));
        if (l < 0)
                return error_origin(-errno);

        c_assert(l == sizeof(si));

        return DISPATCH_E_EXIT;
#endif
}

int broker_new(Broker **brokerp, Log *log, const char *machine_id, int controller_fd, uint64_t max_bytes, uint64_t max_fds, uint64_t max_matches, uint64_t max_objects) {
        _c_cleanup_(broker_freep) Broker *broker = NULL;
        struct ucred ucred;
        socklen_t z;
        int r;

        z = sizeof(ucred);
        r = getsockopt(controller_fd, SOL_SOCKET, SO_PEERCRED, &ucred, &z);
        if (r < 0)
                return error_origin(-errno);

        broker = calloc(1, sizeof(*broker));
        if (!broker)
                return error_origin(-ENOMEM);

        broker->log = log;
        broker->bus = (Bus)BUS_NULL(broker->bus);
        broker->dispatcher = (DispatchContext)DISPATCH_CONTEXT_NULL(broker->dispatcher);
        broker->signals_fd = -1;
        broker->signals_file = (DispatchFile)DISPATCH_FILE_NULL(broker->signals_file);
        broker->controller = (Controller)CONTROLLER_NULL(broker->controller);

        r = bus_init(&broker->bus, broker->log, machine_id, max_bytes, max_fds, max_matches, max_objects);

        if (r)
                return error_fold(r);

        /*
         * We need the seclabel to run the broker for 2 reasons: First, if
         * 'org.freedesktop.DBus' is queried for the seclabel, we need to
         * return some value. Second, all unlabeled names get this label
         * assigned by default. Due to the latter, this seclabel is actually
         * referenced in selinux rules, to allow peers to own names.
         * We use SO_PEERSEC on the controller socket to get this label.
         * However, note that this used to return the 'unlabeled_t' entry for
         * socketpairs until kernel v4.17. From v4.17 onwards it now returns
         * the correct label. There is no way to detect this at runtime,
         * though. We hard-require 4.17. If you use older kernels, you will get
         * selinux denials.
         */
        r = sockopt_get_peersec(controller_fd, &broker->bus.seclabel, &broker->bus.n_seclabel);
        if (r)
                return error_fold(r);

        r = sockopt_get_peergroups(controller_fd,
                                   broker->log,
                                   ucred.uid,
                                   ucred.gid,
                                   &broker->bus.gids,
                                   &broker->bus.n_gids);
        if (r)
                return error_fold(r);

        broker->bus.pid = ucred.pid;
        r = user_registry_ref_user(&broker->bus.users, &broker->bus.user, ucred.uid);
        if (r)
                return error_fold(r);

        r = sockopt_get_peerpidfd(controller_fd, &broker->bus.pid_fd);
        if (r) {
                if (r != SOCKOPT_E_UNSUPPORTED &&
                    r != SOCKOPT_E_UNAVAILABLE &&
                    r != SOCKOPT_E_REAPED)
                        return error_fold(r);

                /* keep `pid_fd == -1` if unavailable */
        }

        LOG_DBG("ucred.pid = %d, ucred.uid = %d, ucred.gid = %d", ucred.pid, ucred.uid, ucred.gid);

        r = dispatch_context_init(&broker->dispatcher);
        if (r)
                return error_fold(r);

#ifdef __ZEPHYR__
        // Broker termination is taken care in dispatch_context_init via pipe
        // if (socketpair(AF_UNIX, SOCK_STREAM, 0, terminate_pipe) == -1) {
        //     return error_origin(-errno);
        // }
        
        // broker->signals_fd = terminate_pipe[0];  // Read end

        // r = dispatch_file_init(&broker->signals_file,
        //                         &broker->dispatcher,
        //                         broker_dispatch_signals,
        //                         broker->signals_fd,
        //                         POLLIN,  // For Zephyr's poll compatibility
        //                         0);
#else
        sigset_t sigmask;
        sigemptyset(&sigmask);
        sigaddset(&sigmask, SIGTERM);
        sigaddset(&sigmask, SIGINT);

        broker->signals_fd = signalfd(-1, &sigmask, SFD_CLOEXEC | SFD_NONBLOCK);
        if (broker->signals_fd < 0)
                return error_origin(-errno);

        r = dispatch_file_init(&broker->signals_file,
                               &broker->dispatcher,
                               broker_dispatch_signals,
                               broker->signals_fd,
                               EPOLLIN,
                               0);
#endif
        if (r)
                return error_fold(r);

#ifndef __ZEPHYR__
        dispatch_file_select(&broker->signals_file, EPOLLIN);
#endif
        r = controller_init(&broker->controller, broker, controller_fd);
        if (r)
                return error_fold(r);

#ifdef __ZEPHYR__
        g_broker = broker;
#endif
        *brokerp = broker;
        broker = NULL;
        return 0;
}

// Function to request broker termination (call this from outside)
void broker_request_terminate(Broker *broker) {
    dispatch_context_terminate(&broker->dispatcher);
}

Broker *broker_free(Broker *broker) {
        if (!broker)
                return NULL;

        controller_deinit(&broker->controller);
#ifndef __ZEPHYR__
        dispatch_file_deinit(&broker->signals_file);
        c_close(broker->signals_fd);
#endif
        dispatch_context_deinit(&broker->dispatcher);
        bus_deinit(&broker->bus);
        free(broker);

        return NULL;
}

static int broker_log_metrics(Broker *broker) {
        Sampler *sampler = &broker->bus.sampler;
        double stddev;
        int r;

        stddev = sampler_read_standard_deviation(sampler);
        log_appendf(broker->bus.log,
                    "DBUS_BROKER_METRICS_DISPATCH_COUNT=%"PRIu64"\n"
                    "DBUS_BROKER_METRICS_DISPATCH_MIN=%"PRIu64"\n"
                    "DBUS_BROKER_METRICS_DISPATCH_MAX=%"PRIu64"\n"
                    "DBUS_BROKER_METRICS_DISPATCH_AVG=%"PRIu64"\n"
                    "DBUS_BROKER_METRICS_DISPATCH_STDDEV=%.0f\n",
                    sampler->count,
                    sampler->minimum,
                    sampler->maximum,
                    sampler->average,
                    stddev);
        log_append_here(broker->bus.log, LOG_INFO, 0, DBUS_BROKER_CATALOG_DISPATCH_STATS);
        r = log_commitf(broker->bus.log,
                       "Dispatched %"PRIu64" messages @ %"PRIu64"(±%.0f)μs / message.",
                       sampler->count,
                       sampler->average / 1000,
                       stddev / 1000);
        if (r)
                return error_fold(r);

        return 0;
}

int broker_run(Broker *broker) {
#ifdef __ZEPHYR__
    // In Zephyr, we handle termination through the event loop
    int r;
    
    r = connection_open(&broker->controller.connection);
    if (r == CONNECTION_E_EOF)
            return MAIN_EXIT;
    else if (r)
            return error_fold(r);

    do {
            r = dispatch_context_dispatch(&broker->dispatcher);
            
            if (r == DISPATCH_E_EXIT)
                    return MAIN_EXIT;
            else if (r == DISPATCH_E_FAILURE)
                    return MAIN_FAILED;
            else
                    r = error_fold(r);
    } while (!r);

    peer_registry_flush(&broker->bus.peers);

#else
    sigset_t signew, sigold;
    int r, k;

    sigemptyset(&signew);
    sigaddset(&signew, SIGTERM);
    sigaddset(&signew, SIGINT);

    sigprocmask(SIG_BLOCK, &signew, &sigold);

    r = connection_open(&broker->controller.connection);
    if (r == CONNECTION_E_EOF)
            return MAIN_EXIT;
    else if (r)
            return error_fold(r);

    do {
            r = dispatch_context_dispatch(&broker->dispatcher);
            if (r == DISPATCH_E_EXIT)
                    r = MAIN_EXIT;
            else if (r == DISPATCH_E_FAILURE)
                    r = MAIN_FAILED;
            else
                    r = error_fold(r);
    } while (!r);

    peer_registry_flush(&broker->bus.peers);

    k = broker_log_metrics(broker);
    if (k)
            r = error_fold(k);

    sigprocmask(SIG_SETMASK, &sigold, NULL);

#endif
    return r;
}

int broker_update_environment(Broker *broker, const char * const *env, size_t n_env) {
        return error_fold(controller_dbus_send_environment(&broker->controller, env, n_env));
}

int broker_reload_config(Broker *broker, User *sender_user, uint64_t sender_id, uint32_t sender_serial) {
        int r;

        r = controller_request_reload(&broker->controller, sender_user, sender_id, sender_serial);
        if (r) {
                if (r == CONTROLLER_E_SERIAL_EXHAUSTED ||
                    r == CONTROLLER_E_QUOTA)
                        return BROKER_E_FORWARD_FAILED;

                return error_fold(r);
        }

        return 0;
}
