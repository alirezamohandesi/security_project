#include "record.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Hard cap on the amount of input we are willing to buffer. */
#define MAX_INPUT_BYTES (1u << 20)

static int read_all(FILE *in, uint8_t **out_data, size_t *out_len)
{
    size_t cap = 4096;
    size_t len = 0;
    uint8_t *buf = malloc(cap);

    if (buf == NULL) {
        return -1;
    }

    for (;;) {
        if (len == cap) {
            if (cap >= MAX_INPUT_BYTES) {
                break;
            }
            cap *= 2;
            uint8_t *grown = realloc(buf, cap);
            if (grown == NULL) {
                free(buf);
                return -1;
            }
            buf = grown;
        }

        size_t got = fread(buf + len, 1, cap - len, in);
        len += got;
        if (got == 0) {
            break;
        }
    }

    *out_data = buf;
    *out_len = len;
    return 0;
}

int main(int argc, char **argv)
{
    FILE *in = stdin;

    if (argc > 2) {
        fprintf(stderr, "usage: recparse [FILE|-]\n");
        return RC_USAGE;
    }

    if (argc == 2 && strcmp(argv[1], "-") != 0) {
        in = fopen(argv[1], "rb");
        if (in == NULL) {
            fprintf(stderr, "error: cannot open '%s'\n", argv[1]);
            return RC_INVALID;
        }
    }

    uint8_t *data = NULL;
    size_t len = 0;

    if (read_all(in, &data, &len) != 0) {
        fprintf(stderr, "error: cannot read input\n");
        if (in != stdin) {
            fclose(in);
        }
        return RC_INVALID;
    }

    if (in != stdin) {
        fclose(in);
    }

    int rc = parse_stream(data, len);

    free(data);
    fflush(stdout);
    return rc;
}
