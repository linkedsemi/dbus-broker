/*
 * Broker - Zephyr-specific modifications
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

/* Keep the original broker_new implementation but add Zephyr-specific parts */
int broker_new(Broker **brokerp, Log *log, const char *machine_id, int controller_fd, uint64_t max_bytes, uint64_t max_fds, uint64_t max_matches, uint64_t max_objects) {
        _c_cleanup_(broker_freep) Broker *broker = NULL;
        int r;
        
#ifdef __ZEPHYR__
        /* Zephyr doesn't support SO_PEERCRED, use default values */
        uid_t uid = 0;
        gid_t gid = 0;
        pid_t pid = 0;
#else
        struct ucred ucred;
        socklen_t z;

        z = sizeof(ucred);
        r = getsockopt(controller_fd, SOL_SOCKET, SO_PEERCRED, &ucred, &z);
        if (r < 0)
                return error_origin(-errno);
#endif

        broker = calloc(1, sizeof(*broker));
        if (!broker)
                return error_origin(-ENOMEM);

        broker->log = log;
        broker->bus = (Bus)BUS_NULL(broker->bus);
        broker->dispatcher = (DispatchContext)DISPATCH_CONTEXT_NULL(broker->dispatcher);
        broker->signals_fd = -1;
        broker->signals_file = (DispatchFile)DISPATCH_FILE_NULL(broker->signals_file);
        broker->controller = (Controller)CONTROLLER_NULL(broker->controller);

        /* Add parameter validation */
        if (!brokerp) {
                LOG_ERR("broker_new: brokerp is NULL");
                return -EINVAL;
        }
        
        if (controller_fd < 0) {
                LOG_ERR("broker_new: Invalid controller_fd: %d", controller_fd);
                return -EINVAL;
        }
        
        r = bus_init(&broker->bus, broker->log, machine_id, max_bytes, max_fds, max_matches, max_objects);

        if (r)
                return error_fold(r);

#ifdef __ZEPHYR__
        /* Zephyr: skip SELinux and peer credential checks */
        broker->bus.seclabel = strdup("unlabeled");
        if (!broker->bus.seclabel)
                return error_origin(-ENOMEM);
        broker->bus.n_seclabel = strlen(broker->bus.seclabel) + 1;
        broker->bus.gids = NULL;
        broker->bus.n_gids = 0;
        broker->bus.pid = pid;
        r = user_registry_ref_user(&broker->bus.users, &broker->bus.user, uid);
        if (r)
                return error_fold(r);
#else
        /*
         * Original Linux implementation for peer credentials
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
#endif

        r = sockopt_get_peerpidfd(controller_fd, &broker->bus.pid_fd);
        if (r) {
                if (r != SOCKOPT_E_UNSUPPORTED &&
                    r != SOCKOPT_E_UNAVAILABLE &&
                    r != SOCKOPT_E_REAPED)
                        return error_fold(r);
                /* keep `pid_fd == -1` if unavailable */
        }

#ifdef __ZEPHYR__
        // LOG_DBG("pid = %d, uid = %d, gid = %d", pid, uid, gid);
#else
        LOG_DBG("ucred.pid = %d, ucred.uid = %d, ucred.gid = %d", ucred.pid, ucred.uid, ucred.gid);
#endif

        r = dispatch_context_init(&broker->dispatcher);
        if (r)
                return error_fold(r);

#ifdef __ZEPHYR__
        /* Zephyr doesn't use signal file descriptors */
        LOG_DBG("Zephyr: Skipping signal fd setup");
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
        if (r)
                return error_fold(r);

        dispatch_file_select(&broker->signals_file, EPOLLIN);
#endif

        r = controller_init(&broker->controller, broker, controller_fd);
        if (r)
                return error_fold(r);

#ifdef __ZEPHYR__
        /* Set global broker instance for external access */
        g_broker = broker;
#endif

        *brokerp = broker;
        broker = NULL;
        return 0;
}

/* Get controller connection for service registration */
#ifdef __ZEPHYR__
Connection* broker_get_controller_connection(Broker *broker) {
        if (!broker)
                return NULL;
        return &broker->controller.connection;
}
#endif

/* Function to request broker termination */
void broker_request_terminate(Broker *broker) {
        if (broker) {
                dispatch_context_terminate(&broker->dispatcher);
        }
}

/* Keep the original broker_free but update global pointer */
Broker *broker_free(Broker *broker) {
        if (!broker)
                return NULL;

#ifdef __ZEPHYR__
        /* Clear global broker instance */
        if (broker == g_broker) {
                g_broker = NULL;
        }
#endif

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

/* Keep the original broker_run implementation but with Zephyr adaptations */
int broker_run(Broker *broker) {
#ifdef __ZEPHYR__
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
                else if (r)
                        r = error_fold(r);
        } while (!r);

        peer_registry_flush(&broker->bus.peers);
        return 0;

#else
        /* Original Linux implementation */
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
        return r;
#endif
}

/* Other existing functions remain unchanged */
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