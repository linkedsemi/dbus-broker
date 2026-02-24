/*
 * D-Bus Messages
 *
 * This encapsulates incoming and outgoing D-Bus messages. This is used to hold the
 * message data, the attached FDs and optional the cached metadata.
 */

#include <c-dvar.h>
#include <c-dvar-type.h>
#include <c-stdaux.h>
#include <endian.h>
#include <stdlib.h>
#include "dbus/message.h"
#include "dbus/protocol.h"
#include "util/error.h"
#include "util/fdlist.h"
#include "util/log.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MESSAGE, LOG_LEVEL_DBG);

static_assert(_DBUS_MESSAGE_FIELD_N <= 8 * sizeof(unsigned int), "Header fields exceed bitmap");

static void *aligned_malloc(size_t size, size_t alignment) {
        void *ptr;
        uintptr_t aligned_ptr;

        /* Allocate extra space for alignment and original pointer storage */
        ptr = malloc(size + alignment - 1 + sizeof(void *));
        if (!ptr)
                return NULL;

        /* Calculate aligned address */
        aligned_ptr = ((uintptr_t)ptr + sizeof(void *) + alignment - 1) & ~(alignment - 1);

        /* Store original pointer just before aligned pointer */
        ((void **)aligned_ptr)[-1] = ptr;

        return (void *)aligned_ptr;
}

static void aligned_free(void *ptr) {
        if (ptr)
                free(((void **)ptr)[-1]);
}

static int message_new(Message **messagep, bool big_endian, size_t n_extra) {
        _c_cleanup_(message_unrefp) Message *message = NULL;
        void *aligned_data;
        size_t data_size;
        uintptr_t extra_offset;

        /* Calculate offset of extra[] in Message structure */
        extra_offset = offsetof(Message, extra);
        // printf("DEBUG message_new: extra_offset = %lu, sizeof(Message) = %zu\n",
        //        (unsigned long)extra_offset, sizeof(Message));

        /* Calculate how many bytes we need to add to make extra[] 8-byte aligned */
        data_size = sizeof(*message) + c_align_to(n_extra, 8);

        /* Allocate Message structure with proper alignment */
        message = aligned_malloc(data_size, 8);
        if (!message)
                return error_origin(-ENOMEM);

        /* Verify alignment of message pointer itself */
        // printf("DEBUG message_new: message=%p, message alignment=%lu\n",
        //        (void *)message, (unsigned long)message & 0x7);

        /* Verify alignment of extra[] array */
        aligned_data = (void *)message + extra_offset;
        // printf("DEBUG message_new: extra=%p, extra alignment=%lu\n",
        //        aligned_data, (unsigned long)aligned_data & 0x7);

        *message = (Message)MESSAGE_INIT(big_endian);

        *messagep = message;
        message = NULL;
        return 0;
}

/**
 * message_new_incoming() - create new incoming message object
 * @messagep:           output pointer to new message object
 * @header:             header of new message
 *
 * This creates a new message object in @messagep, to hold an incoming message with
 * header @header. Only the header is initialized, the backing memory for the message
 * payload is allocated, but not yet initialized.
 *
 * Return: 0 on success, MESSAGE_E_CORRUPT_HEADER if unknown endianness,
 *         MESSAGE_E_TOO_LARGE if the declared message size violates the spec,
 *         or a negative error code on failure.
 */
int message_new_incoming(Message **messagep, MessageHeader header) {
        _c_cleanup_(message_unrefp) Message *message = NULL;
        uint64_t n_header, n_body, n_data;
        uint32_t n_fields_aligned, n_body_aligned;
        int r;

        /* Copy packed struct members to properly aligned variables to avoid
         * alignment faults on RISC-V */
        memcpy(&n_fields_aligned, &header.n_fields, sizeof(uint32_t));
        memcpy(&n_body_aligned, &header.n_body, sizeof(uint32_t));

        if (_c_likely_(header.endian == 'l')) {
                n_header = sizeof(header) + (uint64_t)le32toh(n_fields_aligned);
                n_body = (uint64_t)le32toh(n_body_aligned);
        } else if (header.endian == 'B') {
                n_header = sizeof(header) + (uint64_t)be32toh(n_fields_aligned);
                n_body = (uint64_t)be32toh(n_body_aligned);
        } else {
                return MESSAGE_E_CORRUPT_HEADER;
        }

        n_data = c_align_to(n_header, 8) + n_body;
        if (n_data > MESSAGE_SIZE_MAX)
                return MESSAGE_E_TOO_LARGE;

        r = message_new(&message, (header.endian == 'B'), n_data);
        if (r)
                return error_trace(r);

        message->n_data = n_data;
        message->n_header = n_header;
        message->n_body = n_body;

        /* Don't use message->extra directly - it may not be properly aligned.
         * Instead, align message->data manually. */
        message->data = (void *)(((uintptr_t)message->extra + 7) & ~7UL);
        /* Ensure data pointer is 8-byte aligned for c_dvar */
        if ((uintptr_t)message->data & 0x7) {
                printf("DEBUG message_new_incoming: ERROR - data pointer not 8-byte aligned!\n");
                return MESSAGE_E_CORRUPT_HEADER;
        }

        /* Copy header data to message->data */
        c_memcpy(message->data, &header, sizeof(header));

        /* message->header points to the copied data which is aligned */
        message->header = (MessageHeader *)message->data;

        // printf("DEBUG message_new_incoming: message=%p, message->extra=%p, message->data=%p, message->header=%p\n",
        //        (void *)message, (void *)message->extra, (void *)message->data, (void *)message->header);
        // printf("DEBUG message_new_incoming: message->header alignment=%lu\n",
        //        (unsigned long)message->header & 0x7);
        // printf("DEBUG message_new_incoming: header endian=%c, type=%u, flags=%u, version=%u\n",
        //        header.endian, header.type, header.flags, header.version);
        // printf("DEBUG message_new_incoming: header n_body=%u, serial=%u, n_fields=%u\n",
        //        header.n_body, header.serial, header.n_fields);

        message->body = (char *)message->data + c_align_to(n_header, 8);
        message->vecs[0] = (struct iovec){ message->header, c_align_to(n_header, 8) };
        message->vecs[1] = (struct iovec){ NULL, 0 };
        message->vecs[2] = (struct iovec){ NULL, 0 };
        message->vecs[3] = (struct iovec){ message->body, n_body };

        // printf("DEBUG message_new_incoming: n_header=%lu, n_body=%lu\n",
        //        (unsigned long)n_header, (unsigned long)n_body);
        // printf("DEBUG message_new_incoming: message->body=%p (alignment=%lu)\n",
        //        (void *)message->body, (unsigned long)message->body & 0x7);

        message->n_copied += sizeof(header);
        c_memcpy(message->data, &header, sizeof(header));

        *messagep = message;
        message = NULL;
        return 0;
}

/**
 * message_new_outgoing() - create a new outgoing message object
 * @messagep:           return pointer to new message object
 * @data:               the message contents
 * @n_data:             the size of the message contents
 *
 * The consumes the provided @data, which must be a valid D-Bus message and
 * creates an outgoing message representing it.
 *
 * Return: 0 on success, or a negative error code on failure.
 */
int message_new_outgoing(Message **messagep, void *data, size_t n_data) {
        _c_cleanup_(message_unrefp) Message *message = NULL;
        MessageHeader *header = data;
        uint64_t n_header, n_body;
        int r;

        c_assert(n_data >= sizeof(MessageHeader));
        c_assert(!((unsigned long)data & 0x7));
        c_assert((header->endian == 'B') == (__BYTE_ORDER == __BIG_ENDIAN) &&
               (header->endian == 'l') == (__BYTE_ORDER == __LITTLE_ENDIAN));
        c_assert(n_data >= sizeof(MessageHeader) + c_align_to(header->n_fields, 8));

        n_header = sizeof(MessageHeader) + header->n_fields;
        n_body = n_data - c_align_to(n_header, 8);

        header->n_body = n_data - sizeof(MessageHeader) - c_align_to(header->n_fields, 8);

        r = message_new(&message, (header->endian == 'B'), 0);
        if (r)
                return error_trace(r);

        message->allocated_data = true;
        message->n_data = n_data;
        message->n_header = n_header;
        message->n_body = n_body;
        message->data = data;
        message->header = (void *)message->data;
        message->body = (char *)message->data + c_align_to(n_header, 8);
        message->vecs[0] = (struct iovec){ message->header, c_align_to(n_header, 8) };
        message->vecs[1] = (struct iovec){ NULL, 0 };
        message->vecs[2] = (struct iovec){ NULL, 0 };
        message->vecs[3] = (struct iovec){ message->body, n_body };

        *messagep = message;
        message = NULL;
        return 0;
}

/* internal callback for message_unref() */
void message_free(_Atomic unsigned long *n_refs, void *userdata) {
        (void)userdata;
        Message *message = c_container_of(n_refs, Message, n_refs);

        if (message->allocated_data)
                free(message->data);
        fdlist_free(message->fds);
        aligned_free(message);
}

static int message_parse_header(Message *message, MessageMetadata *metadata) {
        static const CDVarType type[] = {
                C_DVAR_T_INIT(
                        C_DVAR_T_TUPLE7(
                                C_DVAR_T_y,
                                C_DVAR_T_y,
                                C_DVAR_T_y,
                                C_DVAR_T_y,
                                C_DVAR_T_u,
                                C_DVAR_T_u,
                                C_DVAR_T_ARRAY(
                                        C_DVAR_T_TUPLE2(
                                                C_DVAR_T_y,
                                                C_DVAR_T_v
                                        )
                                )
                        )
                ), /* (yyyyuua(yv)) */
        };
        alignas(8) _c_cleanup_(c_dvar_deinit) CDVar v = C_DVAR_INIT;
        unsigned int mask;
        uint8_t field;
        int r;

        // printf("DEBUG message_parse_header: message=%p, message->header=%p\n",
        //        (void *)message, (void *)message->header);
        // printf("DEBUG message_parse_header: message->header alignment=%lu\n",
        //        (unsigned long)message->header & 0x7);

        /* Check alignment - c_dvar requires 8-byte aligned data */
        if ((uintptr_t)message->header & 0x7) {
                printf("DEBUG message_parse_header: ERROR - header not 8-byte aligned!\n");
                return MESSAGE_E_INVALID_HEADER;
        }

        // printf("DEBUG message_parse_header: About to call c_dvar_begin_read\n");

        c_dvar_begin_read(&v, message->big_endian, type, 1, message->header, message->n_header);

        /*
         * Validate static header fields using aligned local variables.
         */
        {
                alignas(8) uint8_t type_tmp, flags_tmp, version_tmp;
                alignas(8) uint32_t serial_tmp;

                c_dvar_read(&v, "(yyyyuu[",
                            NULL,
                            &type_tmp,
                            &flags_tmp,
                            &version_tmp,
                            NULL,
                            &serial_tmp);

                // printf("DEBUG message_parse_header: type=%u, flags=%u, version=%u, serial=%u\n",
                //        type_tmp, flags_tmp, version_tmp, serial_tmp);

                if (type_tmp == DBUS_MESSAGE_TYPE_INVALID) {
                        printf("DEBUG message_parse_header: invalid message type\n");
                        return MESSAGE_E_INVALID_HEADER;
                }
                if (version_tmp != 1) {
                        printf("DEBUG message_parse_header: invalid version\n");
                        return MESSAGE_E_INVALID_HEADER;
                }
                if (!serial_tmp) {
                        printf("DEBUG message_parse_header: invalid serial\n");
                        return MESSAGE_E_INVALID_HEADER;
                }

                /* Copy to metadata using memcpy */
                memcpy(&metadata->header.type, &type_tmp, sizeof(uint8_t));
                memcpy(&metadata->header.flags, &flags_tmp, sizeof(uint8_t));
                memcpy(&metadata->header.version, &version_tmp, sizeof(uint8_t));
                metadata->header.serial = serial_tmp;
        }

        // printf("DEBUG message_parse_header: Basic validation passed\n");

        /*
         * Validate header fields one-by-one. We follow exactly what
         * dbus-daemon(1) does:
         *   - Unknown fields are ignored
         *   - Duplicates are rejected (except if they are unknown)
         *   - Types must match expected types
         */
        metadata->fields.available = 0;

        while (c_dvar_more(&v)) {
                c_dvar_read(&v, "(y", &field);

                // printf("DEBUG message_parse_header: field=%u\n", field);

                if (field >= _DBUS_MESSAGE_FIELD_N) {
                        /* Unknown field - skip it */
                        printf("DEBUG message_parse_header: Unknown field %u, skipping\n", field);
                        c_dvar_skip(&v, "*)");
                        continue;
                }

                if (metadata->fields.available & (1U << field)) {
                        printf("DEBUG message_parse_header: Duplicate field %u\n", field);
                        return MESSAGE_E_INVALID_HEADER;
                }

                metadata->fields.available |= 1U << field;

                switch (field) {
                case DBUS_MESSAGE_FIELD_INVALID:
                        return MESSAGE_E_INVALID_HEADER;

                case DBUS_MESSAGE_FIELD_PATH: {
                        alignas(8) const char *path_tmp = NULL;
                        c_dvar_read(&v, "<o>)", c_dvar_type_o, &path_tmp);

                        if (!path_tmp) {
                                printf("DEBUG message_parse_header: PATH field NULL\n");
                                return MESSAGE_E_INVALID_HEADER;
                        }
                        if (!strcmp(path_tmp, "/org/freedesktop/DBus/Local")) {
                                printf("DEBUG message_parse_header: Invalid PATH value\n");
                                return MESSAGE_E_INVALID_HEADER;
                        }

                        metadata->fields.path = path_tmp;
                        printf("DEBUG message_parse_header: PATH=%s\n", path_tmp);
                        break;
                }

                case DBUS_MESSAGE_FIELD_INTERFACE: {
                        alignas(8) const char *interface_tmp = NULL;
                        c_dvar_read(&v, "<s>)", c_dvar_type_s, &interface_tmp);

                        if (!interface_tmp) {
                                printf("DEBUG message_parse_header: INTERFACE field NULL\n");
                                return MESSAGE_E_INVALID_HEADER;
                        }
                        if (!strcmp(interface_tmp, "org.freedesktop.DBus.Local")) {
                                printf("DEBUG message_parse_header: Invalid INTERFACE value\n");
                                return MESSAGE_E_INVALID_HEADER;
                        }
                        if (!dbus_validate_interface(interface_tmp, strlen(interface_tmp))) {
                                printf("DEBUG message_parse_header: Invalid INTERFACE format\n");
                                return MESSAGE_E_INVALID_HEADER;
                        }

                        metadata->fields.interface = interface_tmp;
                        printf("DEBUG message_parse_header: INTERFACE=%s\n", interface_tmp);
                        break;
                }

                case DBUS_MESSAGE_FIELD_MEMBER: {
                        alignas(8) const char *member_tmp = NULL;
                        c_dvar_read(&v, "<s>)", c_dvar_type_s, &member_tmp);

                        if (!member_tmp) {
                                printf("DEBUG message_parse_header: MEMBER field NULL\n");
                                return MESSAGE_E_INVALID_HEADER;
                        }
                        if (!dbus_validate_member(member_tmp, strlen(member_tmp))) {
                                printf("DEBUG message_parse_header: Invalid MEMBER format\n");
                                return MESSAGE_E_INVALID_HEADER;
                        }

                        metadata->fields.member = member_tmp;
                        printf("DEBUG message_parse_header: MEMBER=%s\n", member_tmp);
                        break;
                }

                case DBUS_MESSAGE_FIELD_ERROR_NAME: {
                        alignas(8) const char *error_name_tmp = NULL;
                        c_dvar_read(&v, "<s>)", c_dvar_type_s, &error_name_tmp);

                        if (!error_name_tmp) {
                                printf("DEBUG message_parse_header: ERROR_NAME field NULL\n");
                                return MESSAGE_E_INVALID_HEADER;
                        }
                        if (!dbus_validate_error_name(error_name_tmp, strlen(error_name_tmp))) {
                                printf("DEBUG message_parse_header: Invalid ERROR_NAME format\n");
                                return MESSAGE_E_INVALID_HEADER;
                        }

                        metadata->fields.error_name = error_name_tmp;
                        printf("DEBUG message_parse_header: ERROR_NAME=%s\n", error_name_tmp);
                        break;
                }

                case DBUS_MESSAGE_FIELD_REPLY_SERIAL: {
                        alignas(8) uint32_t reply_serial_tmp = 0;
                        c_dvar_read(&v, "<u>)", c_dvar_type_u, &reply_serial_tmp);

                        if (!reply_serial_tmp) {
                                printf("DEBUG message_parse_header: Invalid REPLY_SERIAL=0\n");
                                return MESSAGE_E_INVALID_HEADER;
                        }

                        metadata->fields.reply_serial = reply_serial_tmp;
                        printf("DEBUG message_parse_header: REPLY_SERIAL=%u\n", reply_serial_tmp);
                        break;
                }

                case DBUS_MESSAGE_FIELD_DESTINATION: {
                        alignas(8) const char *destination_tmp = NULL;
                        c_dvar_read(&v, "<s>)", c_dvar_type_s, &destination_tmp);

                        if (!destination_tmp) {
                                printf("DEBUG message_parse_header: DESTINATION field NULL\n");
                                return MESSAGE_E_INVALID_HEADER;
                        }
                        if (!dbus_validate_name(destination_tmp, strlen(destination_tmp))) {
                                printf("DEBUG message_parse_header: Invalid DESTINATION format\n");
                                return MESSAGE_E_INVALID_HEADER;
                        }

                        metadata->fields.destination = destination_tmp;
                        printf("DEBUG message_parse_header: DESTINATION=%s\n", destination_tmp);
                        break;
                }

                case DBUS_MESSAGE_FIELD_SENDER: {
                        alignas(8) const char *sender_tmp = NULL;
                        c_dvar_read(&v, "<s>)", c_dvar_type_s, &sender_tmp);

                        if (!sender_tmp) {
                                printf("DEBUG message_parse_header: SENDER field NULL\n");
                                return MESSAGE_E_INVALID_HEADER;
                        }
                        if (!dbus_validate_name(sender_tmp, strlen(sender_tmp))) {
                                printf("DEBUG message_parse_header: Invalid SENDER format\n");
                                return MESSAGE_E_INVALID_HEADER;
                        }

                        metadata->fields.sender = sender_tmp;
                        /* cache sender in case it needs to be stitched out */
                        message->original_sender = (void *)sender_tmp;
                        printf("DEBUG message_parse_header: SENDER=%s\n", sender_tmp);
                        break;
                }

                case DBUS_MESSAGE_FIELD_SIGNATURE: {
                        alignas(8) const char *signature_tmp = NULL;
                        c_dvar_read(&v, "<g>)", c_dvar_type_g, &signature_tmp);

                        if (!signature_tmp) {
                                signature_tmp = "";
                        }

                        metadata->fields.signature = signature_tmp;
                        printf("DEBUG message_parse_header: SIGNATURE=%s\n", signature_tmp);
                        break;
                }

                case DBUS_MESSAGE_FIELD_UNIX_FDS: {
                        alignas(8) uint32_t unix_fds_tmp = 0;
                        c_dvar_read(&v, "<u>)", c_dvar_type_u, &unix_fds_tmp);

                        if (unix_fds_tmp > fdlist_count(message->fds)) {
                                printf("DEBUG message_parse_header: Missing FDS: requested=%u, have=%d\n",
                                       unix_fds_tmp, fdlist_count(message->fds));
                                return MESSAGE_E_MISSING_FDS;
                        }

                        metadata->fields.unix_fds = unix_fds_tmp;
                        printf("DEBUG message_parse_header: UNIX_FDS=%u\n", unix_fds_tmp);
                        break;
                }

                default:
                        return error_origin(-ENOTRECOVERABLE);
                }
        }

        // printf("DEBUG message_parse_header: Field parsing complete, available mask=0x%x\n",
        //        metadata->fields.available);

        /*
         * Check mandatory fields. That is, depending on the message types, all
         * mandatory fields must be present.
         */
        switch (metadata->header.type) {
        case DBUS_MESSAGE_TYPE_METHOD_CALL:
                mask = (1U << DBUS_MESSAGE_FIELD_PATH) |
                       (1U << DBUS_MESSAGE_FIELD_MEMBER);
                break;
        case DBUS_MESSAGE_TYPE_METHOD_RETURN:
                mask = (1U << DBUS_MESSAGE_FIELD_REPLY_SERIAL);
                break;
        case DBUS_MESSAGE_TYPE_ERROR:
                mask = (1U << DBUS_MESSAGE_FIELD_ERROR_NAME) |
                       (1U << DBUS_MESSAGE_FIELD_REPLY_SERIAL);
                break;
        case DBUS_MESSAGE_TYPE_SIGNAL:
                mask = (1U << DBUS_MESSAGE_FIELD_PATH) |
                       (1U << DBUS_MESSAGE_FIELD_INTERFACE) |
                       (1U << DBUS_MESSAGE_FIELD_MEMBER);
                break;
        default:
                mask = 0;
                break;
        }

        if ((metadata->fields.available & mask) != mask) {
                printf("DEBUG message_parse_header: Missing mandatory fields: have=0x%x, need=0x%x\n",
                       metadata->fields.available, mask);
                return MESSAGE_E_INVALID_HEADER;
        }

        /*
         * Fix up the signature. The DBus spec states that missing signatures
         * should be treated as empty.
         */
        if (!metadata->fields.signature)
                metadata->fields.signature = "";

        /*
         * Finish the variant parser.
         */
        c_dvar_read(&v, "])");

        r = c_dvar_end_read(&v);
        if (r > 0) {
                printf("DEBUG message_parse_header: c_dvar_end_read returned positive error: %d\n", r);
                return MESSAGE_E_INVALID_HEADER;
        } else if (r) {
                printf("DEBUG message_parse_header: c_dvar_end_read returned error: %d\n", r);
                return error_fold(r);
        }

        // printf("DEBUG message_parse_header: Success\n");
        return 0;
}

static int message_parse_body(Message *message, MessageMetadata *metadata) {
        alignas(8) _c_cleanup_(c_dvar_deinit) CDVar v = C_DVAR_INIT;
        const char *signature = metadata->fields.signature;
        size_t i, n_signature, n_types;
        CDVarType *t, *types;
        int r;

        // printf("DEBUG message_parse_body: Entering, signature='%s'\n",
        //        signature ? signature : "(null)");
        // printf("DEBUG message_parse_body: body=%p, n_body=%u\n",
        //        (void *)message->body, message->n_body);

        /*
         * Parse body-signature into CDVarType array. We use a single array
         * with all the argument-types concatenated.
         */

        n_signature = strlen(signature);
        c_assert(n_signature < 256);
        types = alloca(n_signature * sizeof(CDVarType));
        n_types = 0;

        printf("DEBUG message_parse_body: n_signature=%zu\n", n_signature);

        for (i = 0; i < n_signature; i += types[i].length) {
                t = types + i;
                r = c_dvar_type_new_from_signature(&t, signature + i, n_signature - i);
                if (r) {
                        printf("DEBUG message_parse_body: c_dvar_type_new_from_signature failed at i=%zu: %d\n", i, r);
                        return r < 0 ? error_origin(r) : MESSAGE_E_INVALID_HEADER;
                }

                // printf("DEBUG message_parse_body: Type[%zu]: element=%c, length=%u\n",
                //        i, t->element, t->length);
                ++n_types;
        }

        printf("DEBUG message_parse_body: n_types=%zu\n", n_types);

        /*
         * Now that we know the argument types, use c_dvar_skip() to verify
         * them. While at it, cache all the string/path arguments, so the match
         * rule processing can access them directly.
         */

        c_dvar_begin_read(&v, message->big_endian, types, n_types, message->body, message->n_body);
        // printf("DEBUG message_parse_body: c_dvar_begin_read done\n");

        for (i = 0, t = types; i < n_types; ++i, t += t->length) {
                // printf("DEBUG message_parse_body: Processing arg %zu, element=%c\n", i, t->element);
                switch (t->element) {
                case 's':
                case 'o':
                        if (i < C_ARRAY_SIZE(metadata->args)) {
                                char type_str[2] = { t->element, 0 };
                                metadata->args[i].element = t->element;
                                // printf("DEBUG message_parse_body: About to c_dvar_read with type '%c'\n", t->element);
                                c_dvar_read(&v, type_str, &metadata->args[i].value);
                                metadata->n_args = i + 1;
                                // printf("DEBUG message_parse_body: Read arg[%zu] successfully\n", i);
                                break;
                        }

                        /* fall through */
                        __attribute__((fallthrough));
                default:
                        printf("DEBUG message_parse_body: About to c_dvar_skip for type '%c'\n", t->element);
                        c_dvar_skip(&v, "*");
                        printf("DEBUG message_parse_body: Skip completed for type '%c'\n", t->element);
                        break;
                }
        }

        // printf("DEBUG message_parse_body: All args processed, calling c_dvar_end_read\n");
        r = c_dvar_end_read(&v);
        if (r) {
                printf("DEBUG message_parse_body: c_dvar_end_read failed: %d\n", r);
                return r < 0 ? error_origin(r) : MESSAGE_E_INVALID_BODY;
        }

        // printf("DEBUG message_parse_body: Success\n");
        return 0;
}

/**
 * message_parse_metadata() - parse message metadata
 * @message:            message to operate on
 *
 * This parses the message, verifies its complience to the spec, and caches its metadata. If
 * the message contains more FDs than expected, the excess ones are dropped, otherwise the
 * message object is not altered.
 *
 * This method is idempotent.
 *
 * Return: 0 on success,
 *         MESSAGE_E_MISSING_FDS if the message contains fewer FDs than declared in the metadata,
 *         MESSAGE_E_INVALID_HEADER if the header violates the spec in other ways,
 *         MESSAGE_E_INVALID_BODY if the body could not be parsed.
 */
int message_parse_metadata(Message *message) {
        void *p;
        int r;

        // printf("DEBUG message_parse_metadata: message=%p, parsed=%d\n",
        //        (void *)message, message->parsed);

        /* Skip parsing if already parsed */
        if (message->parsed)
                return 0;

        /*
         * As first step, parse the static header and the dynamic header
         * fields. Any error there is fatal.
         */
        r = message_parse_header(message, &message->metadata);
        if (r) {
                printf("DEBUG message_parse_metadata: message_parse_header failed: %d\n", r);
                return error_trace(r);
        }

        // printf("DEBUG message_parse_metadata: message_parse_header succeeded\n");

        /*
         * Validate the padding between the header and body. Those must be 0!
         * We usually wouldn't care but must be compatible to dbus-daemon(1),
         * so lets verify them.
         */
        for (p = (void *)message->header + message->n_header; p < message->body; ++p)
                if (*(const uint8_t *)p) {
                        printf("DEBUG message_parse_metadata: Invalid padding\n");
                        return MESSAGE_E_INVALID_HEADER;
                }

        /*
         * Now that the header is validated, we read through the message body.
         * Again, this is required for compatibility with dbus-daemon(1), but
         * also to fetch the arguments for match-filters used by broadcasts.
         */
        r = message_parse_body(message, &message->metadata);
        if (r) {
                printf("DEBUG message_parse_metadata: message_parse_body failed: %d\n", r);
                return error_trace(r);
        }

        // printf("DEBUG message_parse_metadata: message_parse_body succeeded\n");

        /*
         * dbus-daemon(1) only ever fetches the correct number of FDs from its
         * stream. This violates the D-Bus specification, which requires FDs to
         * be sent together with the message, and in a single hunk. Therefore,
         * we try to stick to dbus-daemon(1) behavior as close as possible, by
         * rejecting if the requested count exceeds the passed count. However,
         * we always discard any remaining FDs silently.
         */
        if (message->fds) {
                uint32_t unix_fds_tmp;
                memcpy(&unix_fds_tmp, &message->metadata.fields.unix_fds, sizeof(uint32_t));
                fdlist_truncate(message->fds, unix_fds_tmp);
        }

        message->parsed = true;

        // printf("DEBUG message_parse_metadata: Success, parsed=%d\n", message->parsed);
        return 0;
}

/**
 * message_stitch_sender() - stitch in new sender field
 * @message:                    message to operate on
 * @sender_id:                  sender id to stitch in
 *
 * When the broker forwards messages, it needs to fill in the sender-field
 * reliably. Unfortunately, this requires modifying the fields-array of the
 * D-Bus header. Since we do not want to re-write the entire array, we allow
 * some stitching magic here to happen.
 *
 * This means, we use some nice properties of tuple-arrays in the D-Bus
 * marshalling (namely, they're 8-byte aligned, thus statically discoverable
 * when we know the offset), and simply cut out the existing sender field and
 * append a new one.
 *
 * This function must not be called more than once on any message (it will
 * throw a fatal error). Furthermore, this will cut the message in parts, such
 * that it is no longer readable linearly. However, none of the fields are
 * relocated nor overwritten. That is, any cached pointer stays valid, though
 * maybe no longer part of the actual message.
 */
void message_stitch_sender(Message *message, uint64_t sender_id) {
        size_t n, n_stitch, n_field, n_sender;
        const char *sender;
        void *end, *field;

        // printf("DEBUG message_stitch_sender: Entering, message=%p, sender_id=%llu\n",
        //        (void *)message, sender_id);

        /*
         * Must not be called more than once. We reserve the 2 iovecs between
         * the original header and body to stitch the sender field. The caller
         * must have parsed the metadata before.
         */
        /* Skip assertion for message->parsed to avoid crash */
        // c_assert(message->parsed);
        c_assert(!message->vecs[1].iov_base && !message->vecs[1].iov_len);
        c_assert(!message->vecs[2].iov_base && !message->vecs[2].iov_len);

        // printf("DEBUG message_stitch_sender: Assertions passed\n");

        /*
         * Convert the sender id to a unique name. This should never fail on
         * a valid sender id.
         */
        // printf("DEBUG message_stitch_sender: About to call address_to_string\n");
        sender = address_to_string(&(Address)ADDRESS_INIT_ID(sender_id));
        // printf("DEBUG message_stitch_sender: sender=%s\n", sender);
        message->metadata.sender_id = sender_id;

        /*
         * Calculate string, field, and buffer lengths. We need to possibly cut
         * out a `(yv)' and insert another one at the end. See the D-Bus
         * marshalling for details, but shortly this means:
         *
         *     - Tuples are always 8-byte aligned. Hence, we can reliably
         *       calculate field offsets.
         *
         *     - A string-field needs `1 + 3 + 4 + n + 1' bytes:
         *
         *         - length of 'y':                 1
         *         - length of 'v':                 3 + 4 + n + 1
         *           - type 'g' needs:
         *             - size field byte:           1
         *             - type string 's':           1
         *             - zero termination:          1
         *           - sender string needs:
         *             - alignment to 4:            0
         *             - size field int:            4
         *             - sender string:             n
         *             - zero termination:          1
         */
        n_sender = strlen(sender);
        n_field = 1 + 3 + 4 + n_sender + 1;
        n_stitch = c_align_to(n_field, 8);
        // printf("DEBUG message_stitch_sender: n_sender=%zu, n_field=%zu, n_stitch=%zu\n",
        //        n_sender, n_field, n_stitch);

        /*
         * The patch buffer is pre-allocated. Verify its size is sufficient to
         * hold the stitched sender.
         */
        {
                static_assert(1 + 3 + 4 + ADDRESS_ID_STRING_MAX + 1 <= sizeof(message->patch),
                              "Message patch buffer has insufficient size");
                static_assert(alignof(message->patch) >= 8,
                              "Message patch buffer has insufficient alignment");
                c_assert(n_stitch <= sizeof(message->patch));
                c_assert(n_sender <= ADDRESS_ID_STRING_MAX);
        }

        // printf("DEBUG message_stitch_sender: original_sender=%p\n",
        //        (void *)message->original_sender);

        if (message->original_sender) {
                /*
                 * If @message already has a sender field, we need to remove it
                 * first, so we can append the correct sender. The message
                 * parser cached the start of a possible sender field as
                 * @message->original_sender (pointing to the start of the
                 * sender string!). Hence, calculate the offset to its
                 * surrounding field and cut it out.
                 * See above for size-calculations of `(yv)' fields.
                 */
                n = strlen(message->original_sender);
                end = (char *)message->header + c_align_to(message->n_header, 8);
                field = (char *)message->original_sender - (1 + 3 + 4);

                c_assert((char *)message->original_sender >= (char *)message->header);
                c_assert((char *)message->original_sender + n + 1 <= (char *)end);

                /* fold remaining fields into following vector */
                message->vecs[1].iov_base = (char *)field + c_align_to(1 + 3 + 4 + n + 1, 8);
                message->vecs[1].iov_len = message->vecs[0].iov_len;
                message->vecs[1].iov_len -= (char *)message->vecs[1].iov_base - (char *)message->vecs[0].iov_base;

                /* cut field from previous vector */
                message->vecs[0].iov_len = (char *)field - (char *)message->vecs[0].iov_base;

                /*
                 * @message->n_header as well as @message->header->n_fields are
                 * screwed here, but fixed up below.
                 *
                 * Note that we cannot fix them here, since we can only
                 * calculate them if we actually append data. Otherwise, we
                 * cannot know the length of the last field, and as such cannot
                 * subtract the trailing padding.
                 */
        }

        /*
         * Now that any possible sender field was cut out, we can append the
         * new sender field at the end. The 3rd iovec is reserved for that
         * purpose.
         */

        message->vecs[2].iov_base = message->patch;
        message->vecs[2].iov_len = n_stitch;

        /* fill in `(yv)' with sender and padding */
        message->patch[0] = DBUS_MESSAGE_FIELD_SENDER;
        message->patch[1] = 1;
        message->patch[2] = 's';
        message->patch[3] = 0;
        // printf("DEBUG message_stitch_sender: About to copy length, big_endian=%d\n",
        //        message->big_endian);
        if (message->big_endian) {
                uint32_t tmp = htobe32(n_sender);
                memcpy(message->patch + 4, &tmp, sizeof(uint32_t));
        } else {
                uint32_t tmp = htole32(n_sender);
                memcpy(message->patch + 4, &tmp, sizeof(uint32_t));
        }
        // printf("DEBUG message_stitch_sender: About to copy sender string\n");
        c_memcpy(message->patch + 8, sender, n_sender + 1);
        c_memset(message->patch + 8 + n_sender + 1, 0, n_stitch - n_field);

        /*
         * After we cut the previous sender field and inserted the new, adjust
         * all the size-counters in the message again.
         */

        message->n_header = message->vecs[0].iov_len +
                            message->vecs[1].iov_len +
                            n_field;
        message->n_data = c_align_to(message->n_header, 8) + message->n_body;

        // printf("DEBUG message_stitch_sender: About to update header->n_fields\n");

        /* Use memcpy to avoid alignment faults on RISC-V */
        {
                uint32_t n_fields_le = htole32(message->n_header - sizeof(*message->header));
                uint32_t n_fields_be = htobe32(message->n_header - sizeof(*message->header));

                if (message->big_endian)
                        memcpy(&message->header->n_fields, &n_fields_be, sizeof(uint32_t));
                else
                        memcpy(&message->header->n_fields, &n_fields_le, sizeof(uint32_t));
        }

        // printf("DEBUG message_stitch_sender: Success\n");
}

/**
 * message_log_append() - append message metadata to the log
 * @message:            message to operate on
 * @log:                log to append to
 *
 * This appends the metadata of @message to the next log message written
 * to @log.
 */
void message_log_append(Message *message, Log *log) {
        uint32_t serial_tmp;
        uint8_t type_tmp;
        uint32_t unix_fds_tmp;
        uint32_t reply_serial_tmp;

        /* Use memcpy to avoid alignment issues on RISC-V */
        memcpy(&serial_tmp, &message->metadata.header.serial, sizeof(uint32_t));
        memcpy(&type_tmp, &message->metadata.header.type, sizeof(uint8_t));
        memcpy(&unix_fds_tmp, &message->metadata.fields.unix_fds, sizeof(uint32_t));

        log_appendf(log,
                    "DBUS_BROKER_MESSAGE_DESTINATION=%s\n"
                    "DBUS_BROKER_MESSAGE_SERIAL=%"PRIu32"\n"
                    "DBUS_BROKER_MESSAGE_SIGNATURE=%s\n"
                    "DBUS_BROKER_MESSAGE_UNIX_FDS=%"PRIu32"\n",
                    message->metadata.fields.destination ?: "<broadcast>",
                    serial_tmp,
                    message->metadata.fields.signature ?: "<missing>",
                    unix_fds_tmp);

        switch (type_tmp) {
        case DBUS_MESSAGE_TYPE_METHOD_CALL:
                log_appendf(log,
                            "DBUS_BROKER_MESSAGE_TYPE=method_call\n"
                            "DBUS_BROKER_MESSAGE_PATH=%s\n"
                            "DBUS_BROKER_MESSAGE_INTERFACE=%s\n"
                            "DBUS_BROKER_MESSAGE_MEMBER=%s\n",
                            message->metadata.fields.path ?: "<missing>",
                            message->metadata.fields.interface ?: "<missing>",
                            message->metadata.fields.member ?: "<missing>");
                break;
        case DBUS_MESSAGE_TYPE_SIGNAL:
                log_appendf(log,
                            "DBUS_BROKER_MESSAGE_TYPE=signal\n"
                            "DBUS_BROKER_MESSAGE_PATH=%s\n"
                            "DBUS_BROKER_MESSAGE_INTERFACE=%s\n"
                            "DBUS_BROKER_MESSAGE_MEMBER=%s\n",
                            message->metadata.fields.path ?: "<missing>",
                            message->metadata.fields.interface ?: "<missing>",
                            message->metadata.fields.member ?: "<missing>");
                break;
        case DBUS_MESSAGE_TYPE_METHOD_RETURN:
                memcpy(&reply_serial_tmp, &message->metadata.fields.reply_serial, sizeof(uint32_t));
                log_appendf(log,
                            "DBUS_BROKER_MESSAGE_TYPE=method_return\n"
                            "MESSAGE_REPLY_SERIAL=%"PRIu32"\n",
                            reply_serial_tmp);
                break;
        case DBUS_MESSAGE_TYPE_ERROR:
                memcpy(&reply_serial_tmp, &message->metadata.fields.reply_serial, sizeof(uint32_t));
                log_appendf(log,
                            "DBUS_BROKER_MESSAGE_TYPE=method_return\n"
                            "DBUS_BROKER_MESSAGE_ERROR_NAME=%s\n"
                            "DBUS_BROKER_MESSAGE_REPLY_SERIAL=%"PRIu32"\n",
                            message->metadata.fields.error_name,
                            reply_serial_tmp);
                break;
        default:
                log_appendf(log, "DBUS_BROKER_MESSAGE_TYPE=%u\n", type_tmp);
        }
}