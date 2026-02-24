#pragma once

/*
 * D-Bus Messages
 */

#include <c-stdaux.h>
#include <stdlib.h>
#include <endian.h>
#include "dbus/address.h"
#include "dbus/protocol.h"
#include "util/ref.h"

typedef struct FDList FDList;
typedef struct Log Log;
typedef struct Message Message;
typedef struct MessageHeader MessageHeader;
typedef struct MessageMetadata MessageMetadata;

/* max message size; taken from spec */
#define MESSAGE_SIZE_MAX (128UL * 1024UL * 1024UL)

/* max patch buffer size; see message_stitch_sender() */
#define MESSAGE_PATCH_MAX (C_ALIGN_TO(1 + 3 + 4 + ADDRESS_ID_STRING_MAX + 1, 8))

enum {
        _MESSAGE_E_SUCCESS,

        MESSAGE_E_CORRUPT_HEADER,
        MESSAGE_E_TOO_LARGE,
        MESSAGE_E_INVALID_HEADER,
        MESSAGE_E_INVALID_BODY,

        MESSAGE_E_MISSING_FDS,
};

struct MessageMetadata {
        struct __attribute__((packed)) {
                uint8_t type;
                uint8_t flags;
                uint8_t version;
                uint32_t serial;
        } header;

        uint64_t sender_id;

        struct {
                unsigned int available;
                const char *path;
                const char *interface;
                const char *member;
                const char *error_name;
                uint32_t reply_serial;
                const char *destination;
                const char *sender;
                const char *signature;
                uint32_t unix_fds;
        } fields;

        struct {
                char element;
                const void *value;
        } args[64];
        size_t n_args;
};

#define MESSAGE_METADATA_INIT {                         \
                .sender_id = ADDRESS_ID_INVALID,        \
        }

struct Message {
        _Atomic unsigned long n_refs;

        bool big_endian : 1;
        bool allocated_data : 1;
        bool parsed : 1;

        FDList *fds;

        size_t n_data;
        size_t n_copied;
        size_t n_header;
        size_t n_body;

        void *data;
        MessageHeader *header;
        alignas(8) MessageMetadata metadata;
        void *body;

        void *original_sender;
        alignas(8) struct iovec vecs[4];
        alignas(8) uint8_t patch[MESSAGE_PATCH_MAX];
        alignas(8) uint8_t extra[];
} __attribute__((aligned(8)));

#define MESSAGE_INIT(_big_endian) {                     \
                .n_refs = REF_INIT,                     \
                .big_endian = _big_endian,              \
                .metadata = MESSAGE_METADATA_INIT,      \
        }

struct MessageHeader {
        uint8_t endian;
        uint8_t type;
        uint8_t flags;
        uint8_t version;
        uint32_t n_body;
        uint32_t serial;
        uint32_t n_fields;
} _c_packed_;

int message_new_incoming(Message **messagep, MessageHeader header);
int message_new_outgoing(Message **messagep, void *data, size_t n_data);
void message_free(_Atomic unsigned long *n_refs, void *userdata);

int message_parse_metadata(Message *message);
void message_stitch_sender(Message *message, uint64_t sender_id);

void message_log_append(Message *message, Log *log);

/* inline helpers */

/**
 * message_ref() - XXX
 */
static inline Message *message_ref(Message *message) {
        if (message)
                ref_inc(&message->n_refs);
        return message;
}

/**
 * message_unref() - XXX
 */
static inline Message *message_unref(Message *message) {
        if (message)
                ref_dec(&message->n_refs, message_free, NULL);
        return NULL;
}

/**
 * message_read_serial() - XXX
 */
static inline uint32_t message_read_serial(Message *message) {
        uint32_t serial;
        uint8_t type, flags;

        /* Use memcpy to access packed struct members on RISC-V */
        memcpy(&type, &message->header->type, sizeof(uint8_t));
        memcpy(&flags, &message->header->flags, sizeof(uint8_t));

        if (type != DBUS_MESSAGE_TYPE_METHOD_CALL ||
            _c_unlikely_(flags & DBUS_HEADER_FLAG_NO_REPLY_EXPECTED))
                return 0;

        memcpy(&serial, &message->header->serial, sizeof(uint32_t));

        if (_c_likely_(!message->big_endian))
                return le32toh(serial);
        else
                return be32toh(serial);
}

/* Helper functions to safely access packed MessageHeader members on RISC-V */
static inline uint8_t message_read_type(Message *message) {
        uint8_t type;
        memcpy(&type, &message->header->type, sizeof(uint8_t));
        return type;
}

static inline uint8_t message_read_flags(Message *message) {
        uint8_t flags;
        memcpy(&flags, &message->header->flags, sizeof(uint8_t));
        return flags;
}

C_DEFINE_CLEANUP(Message *, message_unref);
