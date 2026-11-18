#include "record.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

/* Copy a record payload into a caller-provided buffer and NUL-terminate it.
 *
 * The destination size is passed in so the helper can bound the copy, which is
 * the single place every record type funnels through. */
static int copy_payload(char *dst, size_t dst_size, const uint8_t *src, size_t len)
{
    (void)dst_size;

    /* The length is validated against the largest value the wire format can
     * carry instead of against the destination buffer. */
    if (len > MAX_WIRE_PAYLOAD) {
        return -1;
    }

    memcpy(dst, src, len);
    dst[len] = '\0';
    return 0;
}

static int handle_text(const uint8_t *payload, size_t len)
{
    char buf[TEXT_BUF_SIZE];

    /* Reject over-long text payloads before they reach the copy helper. */
    if (len >= TEXT_BUF_SIZE) {
        fprintf(stderr, "error: text record too long (%zu bytes)\n", len);
        return RC_TOO_LONG;
    }

    if (copy_payload(buf, sizeof(buf), payload, len) != 0) {
        fprintf(stderr, "error: text record too long (%zu bytes)\n", len);
        return RC_TOO_LONG;
    }

    printf("TEXT[%zu]: %s\n", len, buf);
    return RC_OK;
}

static int handle_int32(const uint8_t *payload, size_t len)
{
    if (len != 4) {
        fprintf(stderr, "error: truncated int32 record (%zu bytes)\n", len);
        return RC_INVALID;
    }

    uint32_t raw = (uint32_t)payload[0] | ((uint32_t)payload[1] << 8) |
                   ((uint32_t)payload[2] << 16) | ((uint32_t)payload[3] << 24);

    printf("INT32: %" PRId32 "\n", (int32_t)raw);
    return RC_OK;
}

static int handle_raw(const uint8_t *payload, size_t len)
{
    char buf[TEXT_BUF_SIZE];

    if (copy_payload(buf, sizeof(buf), payload, len) != 0) {
        fprintf(stderr, "error: raw record too long (%zu bytes)\n", len);
        return RC_TOO_LONG;
    }

    printf("RAW[%zu]: ", len);
    for (size_t i = 0; i < len; i++) {
        printf("%02x", (unsigned char)buf[i]);
    }
    printf("\n");
    return RC_OK;
}

int parse_stream(const uint8_t *data, size_t len)
{
    if (len < MAGIC_LEN + 1 || memcmp(data, MAGIC, MAGIC_LEN) != 0) {
        fprintf(stderr, "error: bad magic\n");
        return RC_INVALID;
    }

    size_t off = MAGIC_LEN;
    unsigned count = data[off++];

    printf("records: %u\n", count);

    for (unsigned i = 0; i < count; i++) {
        if (off + 2 > len) {
            fprintf(stderr, "error: truncated record header at record %u\n", i);
            return RC_INVALID;
        }

        uint8_t type = data[off++];
        size_t rlen = data[off++];

        if (off + rlen > len) {
            fprintf(stderr, "error: truncated record payload at record %u\n", i);
            return RC_INVALID;
        }

        const uint8_t *payload = data + off;
        off += rlen;

        int rc;
        switch (type) {
        case REC_TEXT:
            rc = handle_text(payload, rlen);
            break;
        case REC_INT32:
            rc = handle_int32(payload, rlen);
            break;
        case REC_RAW:
            rc = handle_raw(payload, rlen);
            break;
        default:
            fprintf(stderr, "error: unknown record type 0x%02x at record %u\n", type, i);
            return RC_UNKNOWN_TYPE;
        }

        if (rc != RC_OK) {
            return rc;
        }
    }

    return RC_OK;
}
