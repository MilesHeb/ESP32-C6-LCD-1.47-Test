#include "elm327_tcp.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>

static int set_blocking(int sock, bool blocking)
{
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) return -1;
    if (blocking) flags &= ~O_NONBLOCK;
    else flags |= O_NONBLOCK;
    return fcntl(sock, F_SETFL, flags);
}

bool elm327_tcp_connect(elm327_tcp_t *elm, int timeout_ms)
{
    if (!elm || !elm->host || elm->port <= 0) return false;

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", elm->port);

    struct addrinfo hints = {0};
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *res = NULL;
    if (getaddrinfo(elm->host, port_str, &hints, &res) != 0 || !res) {
        return false;
    }

    int s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (s < 0) {
        freeaddrinfo(res);
        return false;
    }

    // Non-blocking connect with timeout
    set_blocking(s, false);
    int rc = connect(s, res->ai_addr, res->ai_addrlen);
    if (rc != 0 && errno != EINPROGRESS) {
        close(s);
        freeaddrinfo(res);
        return false;
    }

    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(s, &wfds);

    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    rc = select(s + 1, NULL, &wfds, NULL, &tv);
    if (rc <= 0) { // timeout or error
        close(s);
        freeaddrinfo(res);
        return false;
    }

    int so_error = 0;
    socklen_t len = sizeof(so_error);
    getsockopt(s, SOL_SOCKET, SO_ERROR, &so_error, &len);
    if (so_error != 0) {
        close(s);
        freeaddrinfo(res);
        return false;
    }

    set_blocking(s, true);
    freeaddrinfo(res);

    elm->sock = s;
    return true;
}

void elm327_tcp_close(elm327_tcp_t *elm)
{
    if (!elm) return;
    if (elm->sock >= 0) {
        close(elm->sock);
        elm->sock = -1;
    }
}

bool elm327_tcp_send_cmd(elm327_tcp_t *elm, const char *cmd)
{
    if (!elm || elm->sock < 0 || !cmd) return false;

    // Ensure commands end with CR
    char buf[128];
    size_t n = strnlen(cmd, sizeof(buf) - 2);
    memcpy(buf, cmd, n);
    buf[n++] = '\r';

    int sent = send(elm->sock, buf, n, 0);
    return sent == (int)n;
}

static int set_recv_timeout(int sock, int timeout_ms)
{
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    return setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

int elm327_tcp_read_until_prompt(elm327_tcp_t *elm, char *out, size_t out_sz, int timeout_ms)
{
    if (!elm || elm->sock < 0 || !out || out_sz < 2) return -1;

    set_recv_timeout(elm->sock, timeout_ms);

    size_t used = 0;
    while (used < out_sz - 1) {
        char c = 0;
        int r = recv(elm->sock, &c, 1, 0);
        if (r == 0) return -1; // closed
        if (r < 0) {
            // timeout is not fatal: return what we have (if any)
            if (errno == EWOULDBLOCK || errno == EAGAIN) break;
            return -1;
        }

        out[used++] = c;

        if (c == '>') break; // ELM prompt indicates end of response
    }

    out[used] = 0;
    return (int)used;
}
