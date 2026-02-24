#pragma once

/*
 * Broker
 */

#include <c-stdaux.h>
#include <stdlib.h>
#include "broker/controller.h"
#include "bus/bus.h"
#include "util/dispatch.h"
#include "util/log.h"

enum {
        _BROKER_E_SUCCESS,

        BROKER_E_FORWARD_FAILED,
};

typedef struct Broker Broker;
typedef struct User User;

struct Broker {
        Log *log;
        Bus bus;
        DispatchContext dispatcher;

        int signals_fd;
        DispatchFile signals_file;

        Controller controller;
};

/* for Zephyr adaption */
/* Broker configuration structure */
typedef struct BrokerConfig {
        uint64_t max_connections;
        uint64_t max_services;
        uint64_t max_match_rules;
        int log_level;
        bool enable_monitor;
        bool skip_authentication;
        bool use_predefined_fds;
} BrokerConfig;

/* Forward declaration for sd_bus (from systemd) */
struct sd_bus;
typedef struct sd_bus sd_bus;

/* broker */

int broker_new(Broker **brokerp, Log *log, const char *machine_id, int controller_fd, uint64_t max_bytes, uint64_t max_fds, uint64_t max_matches, uint64_t max_objects);
Broker *broker_free(Broker *broker);

int broker_run(Broker *broker);
int broker_update_environment(Broker *broker, const char * const *env, size_t n_env);
int broker_reload_config(Broker *broker, User *sender_user, uint64_t sender_id, uint32_t sender_serial);
void broker_request_terminate(Broker *broker); // call from app to terminate broker

int broker_set_config(Broker *broker, const BrokerConfig *config);
int broker_set_controller_fd(Broker *broker, int fd);
sd_bus* broker_get_internal_bus(Broker *broker);

C_DEFINE_CLEANUP(Broker *, broker_free);

#ifdef __ZEPHYR__
/* For service registration, expose the controller connection */
Connection* broker_get_controller_connection(Broker *broker);

/* Request graceful broker termination */
void broker_request_terminate(Broker *broker);

/* Global broker instance for external access */
extern Broker *g_broker;
#endif

/* inline helpers */

static inline Broker *BROKER(Bus *bus) {
        /*
         * This function up-casts a Bus to its parent class Broker. In our code
         * base we pretend a Bus is an abstract class with several virtual
         * methods. However, we only do this to clearly separate our code
         * bases. We never intended this to be modular. Hence, instead of
         * providing real vtables with userdata pointers, we instead allow
         * explicit up-casts to the parent type.
         *
         * This function performs the up-cast, relying on the fact that all our
         * Bus objects are always owned by a Broker object.
         */
        return c_container_of(bus, Broker, bus);
}
