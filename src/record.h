/* recparse — RECV1 record parser.
 *
 * See SPECIFICATION.md for the authoritative description of the format,
 * the output contract and the exit codes.
 */
#ifndef RECPARSE_RECORD_H
#define RECPARSE_RECORD_H

#include <stddef.h>
#include <stdint.h>

/* Exit codes — part of the public contract (SPECIFICATION.md section 5). */
#define RC_OK 0
#define RC_INVALID 1
#define RC_TOO_LONG 2
#define RC_UNKNOWN_TYPE 3
#define RC_USAGE 64

/* Record type tags (SPECIFICATION.md section 3). */
#define REC_TEXT 0x01
#define REC_INT32 0x02
#define REC_RAW 0x03

#define MAGIC "RECV1"
#define MAGIC_LEN 5

/* Destination buffer size used for TEXT and RAW payloads.  The payload is
 * stored NUL-terminated, so the largest accepted length is TEXT_BUF_SIZE - 1. */
#define TEXT_BUF_SIZE 32

/* Largest length representable by the on-the-wire length field. */
#define MAX_WIRE_PAYLOAD 255

/* Parse a whole RECV1 stream.  Returns one of the RC_* codes. */
int parse_stream(const uint8_t *data, size_t len);

#endif /* RECPARSE_RECORD_H */
