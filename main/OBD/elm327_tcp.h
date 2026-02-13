#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    const char *host;   // e.g. "192.168.0.10"
    int port;           // e.g. 35000
    int sock;           // internal
} elm327_tcp_t;

bool elm327_tcp_connect(elm327_tcp_t *elm, int timeout_ms);
void elm327_tcp_close(elm327_tcp_t *elm);

bool elm327_tcp_send_cmd(elm327_tcp_t *elm, const char *cmd);

/**
 * Read ELM response until '>' prompt or timeout.
 * Returns number of bytes in out (null-terminated), or -1 on error.
 */
int elm327_tcp_read_until_prompt(elm327_tcp_t *elm, char *out, size_t out_sz, int timeout_ms);
