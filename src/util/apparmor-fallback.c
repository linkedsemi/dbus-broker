/*
 * Bus AppArmor Fallback Helpers
 *
 * This fallback is used when libapparmor is not available, and is meant to be
 * functionally equivalent to util/apparmor.c in case AppArmor is disabled.
 *
 * See util/apparmor.c for details.
 */

#include <c-stdaux.h>
#include <stdio.h>
#include <stdlib.h>
#include "util/apparmor.h"

/**
 * bus_apparmor_is_enabled() - checks if AppArmor is currently enabled
 * @enabled:            return argument telling if AppArmor is enabled
 *
 * If the AppArmor module is not loaded, or AppArmor is disabled in the
 * kernel, set @enabledp to 'false', otherwise set it to 'true'.
 *
 * Returns: 0 if check succeeded, or negative error code on failure.
 */
int bus_apparmor_is_enabled(bool *enabledp) {
        *enabledp = false;
        return 0;
}

/**
 * bus_apparmor_dbus_supported() - check for apparmor dbus support
 * @supported:            return argument telling if AppArmor DBus is supported
 *
 * If the AppArmor module is not loaded, or AppArmor does not support DBus,
 * set @supportedp to 'false', otherwise set it to 'true'.
 *
 * Returns: 0 if check succeeded, or negative error code on failure.
 */
int bus_apparmor_dbus_supported(bool *supportedp) {
        *supportedp = false;
        return 0;
}

int bus_apparmor_registry_new(struct BusAppArmorRegistry **registryp, const char *fallback_context) {
        (void)fallback_context;
        *registryp = NULL;
        return 0;
}

BusAppArmorRegistry *bus_apparmor_registry_ref(BusAppArmorRegistry *registry) {
        (void)registry;
        return NULL;
}

BusAppArmorRegistry *bus_apparmor_registry_unref(BusAppArmorRegistry *registry) {
        (void)registry;
        return NULL;
}

int bus_apparmor_set_bus_type(BusAppArmorRegistry *registry, const char *bustype) {
        (void)registry;
        (void)bustype;
        return 0;
}

int bus_apparmor_check_own(struct BusAppArmorRegistry *registry, const char *owner_context,
                           const char *name) {
        (void)registry;
        (void)owner_context;
        (void)name;
        return 0;
}

int bus_apparmor_check_send(BusAppArmorRegistry *registry,
                            const char *sender_context, const char *receiver_context,
                            NameSet *subject, uint64_t subject_id,
                            const char *path, const char *interface, const char *method) {
        (void)registry;
        (void)sender_context;
        (void)receiver_context;
        (void)subject;
        (void)subject_id;
        (void)path;
        (void)interface;
        (void)method;
        return 0;
}

int bus_apparmor_check_eavesdrop(BusAppArmorRegistry *registry, const char *context) {
        (void)registry;
        (void)context;
        return 0;
}
