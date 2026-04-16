# dbus-broker - High-Performance D-Bus Message Broker for Zephyr

## Overview

dbus-broker is a high-performance D-Bus message broker implementation, ported to work with Zephyr RTOS. It provides a central message bus for inter-process communication in embedded systems.

## Module Structure

```
dbus-broker/
├── CMakeLists.txt          # Main build configuration (modular)
├── zephyr/
│   ├── module.yml          # Zephyr module definition
│   ├── Kconfig             # Configuration options
│   ├── CMakeLists.txt      # Zephyr-specific build (legacy, kept for compatibility)
│   ├── include/            # Public headers and compatibility layer
│   │   ├── sys/            # System call compatibility
│   │   └── linux/          # Linux compatibility headers
│   └── tool/               # Zephyr compatibility shims
├── src/                    # Source code
│   ├── broker/             # Broker core implementation
│   ├── bus/                # Bus management
│   ├── dbus/               # D-Bus protocol layer
│   ├── util/               # Utility functions
│   └── catalog/            # Catalog IDs
├── subprojects/            # Third-party dependencies
│   ├── libcdvar-1/         # D-Bus variant handling
│   ├── libclist-3/         # Intrusive linked list
│   ├── libcrbtree-3/       # Red-black tree
│   ├── libcstdaux-1/       # C standard auxiliaries
│   └── libcutf8-1/         # UTF-8 handling
└── test/                   # Test suite
```

## Configuration Options

Enable dbus-broker in your Zephyr project by adding to `prj.conf`:

```kconfig
CONFIG_DBUS_BROKER=y
CONFIG_DBUS_BROKER_LOG_LEVEL=3              # 0=OFF, 1=ERROR, 2=WARNING, 3=INFO, 4=DEBUG
CONFIG_DBUS_BROKER_MAX_PEERS=64             # Maximum concurrent connections
CONFIG_DBUS_BROKER_MAX_NAMES=128            # Maximum well-known names
# Optional features (not fully supported in Zephyr)
# CONFIG_DBUS_BROKER_ENABLE_POLICY=y
# CONFIG_DBUS_BROKER_ENABLE_LAUNCH=y
```

## Features

- **High Performance**: Optimized for low-latency message routing
- **D-Bus Standard Compliance**: Implements the D-Bus specification
- **Modular Architecture**: Separated into protocol, bus, and broker layers
- **Zephyr Integration**: Native integration with Zephyr's networking and event system

## Dependencies

- **basu**: sd-bus client library (automatically selected)
- **Zephyr Networking**: `CONFIG_NET_SOCKETS=y`
- **POSIX API**: `CONFIG_POSIX_API=y`
- **File System** (optional): `CONFIG_FILE_SYSTEM=y`

## Architecture

```
Application Layer
    ↓ (sd-bus API from basu)
Broker Layer (src/broker/)
    ↓
Bus Management Layer (src/bus/)
    ↓
D-Bus Protocol Layer (src/dbus/)
    ↓
Socket/Network Layer (Zephyr NET API)
```

### Components

1. **Broker Core** (`src/broker/`)
   - Main broker logic
   - Connection management
   - Controller interface

2. **Bus Management** (`src/bus/`)
   - Peer management
   - Name registry
   - Match rules
   - Reply tracking
   - Metrics collection

3. **D-Bus Protocol** (`src/dbus/`)
   - Message serialization/deserialization
   - Connection handling
   - SASL authentication
   - Socket I/O

4. **Utilities** (`src/util/`)
   - Dispatch/event loop
   - File descriptor lists
   - Logging
   - String utilities

## Usage Example

### Starting the Broker

```c
#include <dbus-broker/broker.h>

struct broker *broker = NULL;
int controller_fd;

// Initialize broker with controller socket pair
broker_new(&broker, controller_fd);

// Run the broker event loop
broker_run(broker);

// Cleanup
broker_free(broker);
```

### Client Connection (using basu)

```c
#include <systemd/sd-bus.h>

sd_bus *bus = NULL;

// Connect to the broker
sd_bus_open_user(&bus);

// Send a message
sd_bus_emit_signal(bus,
    "/org/example/Object",
    "org.example.Interface",
    "SignalName",
    "s",
    "Hello D-Bus!");

sd_bus_unref(bus);
```

## Recent Improvements

### Modular Build System

The CMakeLists.txt has been restructured for better modularity:

- **Conditional Compilation**: Features can be enabled/disabled via Kconfig
- **Clear Dependencies**: Subproject dependencies are explicitly declared
- **Configurable Limits**: Maximum peers and names configurable at build time
- **Log Level Control**: Runtime log level selection

### Socket Write Reliability

Integrated with basu's improved socket write mechanism:

- Automatic retry on transient failures
- Proper EAGAIN handling in Zephyr environment
- Clean error reporting

## Building

The module integrates with Zephyr's build system:

```bash
west build -b <your_board> <your_app>
```

## Testing

Test suites are available in the `test/` directory:

```bash
# Unit tests
cd test/dbus
make check

# Integration tests
cd test/integration
./run-tests.sh
```

## Limitations in Zephyr

The following features are currently disabled or limited:

- **Policy Support**: Not fully implemented (`CONFIG_DBUS_BROKER_ENABLE_POLICY=n`)
- **Launch Daemon**: Service activation not supported (`CONFIG_DBUS_BROKER_ENABLE_LAUNCH=n`)
- **Security Modules**: AppArmor, SELinux, Audit not available
- **System Console Users**: No multi-user support

## Performance Tuning

Adjust these parameters based on your requirements:

```kconfig
# Increase for more concurrent clients
CONFIG_DBUS_BROKER_MAX_PEERS=128

# Increase for more service registrations
CONFIG_DBUS_BROKER_MAX_NAMES=256

# Adjust log level for production (lower = less overhead)
CONFIG_DBUS_BROKER_LOG_LEVEL=2
```

## License

SPDX-License-Identifier: Apache-2.0 / MIT

## References

- [D-Bus Specification](https://dbus.freedesktop.org/doc/dbus-specification.html)
- [dbus-broker upstream](https://github.com/bus1/dbus-broker)
- [Zephyr Project](https://www.zephyrproject.org/)
- [basu sd-bus library](../basu/README.md)
