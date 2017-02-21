#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <event2/event.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE 16384

void read_cb(evutil_socket_t fd, short events, void* arg);

struct fd_state {
    struct in_addr mcast_ip;
    u_int16_t mcast_port;

    struct event* read_event;

    struct stream_buffer *buffer;
};

struct fd_state* alloc_rx_conn(struct event_base* base, evutil_socket_t fd)
{
    struct fd_state* state = malloc(sizeof(struct fd_state));
    if (!state)
        return NULL;

    state->read_event = event_new(base, fd, EV_READ | EV_PERSIST, read_cb, state);

    if (!state->read_event) {
        free(state);
        return NULL;
    }

    return state;
}

void free_fd_state(struct fd_state* state)
{
    event_free(state->read_event);
    free(state);
}

int mcast_stream_create(struct event_base* base, struct in_addr mcast, struct in_addr local, u_int16_t mcast_port)
{
    evutil_socket_t nsock = socket(AF_INET, SOCK_DGRAM, 0);
    evutil_make_socket_nonblocking(nsock);

    {
        int one = 1;
        setsockopt(nsock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    }

    {
        struct sockaddr_in sin;

        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_port = mcast_port;

        if (bind(nsock, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
            fprintf(stderr, "bind(): failed.\n");
            return -1;
        }
    }

    {
        int loop = 1;
        if (setsockopt(nsock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0) {
            fprintf(stderr, "setsockopt(): loop failed.\n");
            return -1;
        }
    }

    struct ip_mreq mreq;

    mreq.imr_multiaddr = mcast;
    mreq.imr_interface = local;

    if (setsockopt(nsock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        fprintf(stderr, "setsockopt(): add membership failed.\n");
        return -1;
    }

    struct fd_state* state = alloc_rx_conn(base, nsock);
    if (!state) {
        fprintf(stderr, "Allocation failed.\n");
        return -1;
    }
    state->mcast_ip = mreq.imr_multiaddr;
    state->mcast_port = mcast_port;

    event_add(state->read_event, NULL);
    return 0;
}

void read_cb(evutil_socket_t fd, short events, void* arg)
{
    (void)events;
    struct fd_state* state = arg;
    char buf[1024];
    struct sockaddr_in src_addr;
    socklen_t len;
    ssize_t result;

    result = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr*)&src_addr, &len);

    printf("[RX]\n");

    if (result == 0) {
        // The sender has closed? This shouldn't happen.
        free_fd_state(state);
    } else if (result < 0) {
        int error = evutil_socket_geterror(fd);
        fprintf(stderr, "recvfrom() error: %s.\n", evutil_socket_error_to_string(error));

        if (error == EVUTIL_EAI_AGAIN)
        {
            return;
        }

        free_fd_state(state);
    }
}

/*
int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    struct in_addr local;

    local.s_addr = INADDR_ANY;

    struct event_base* base = event_base_new();
    if (!base)
    {
        fprintf(stderr, "Error creating event loop.\n");
        return -1;
    }

    struct in_addr mcast;

    mcast.s_addr = inet_addr("224.0.0.251");
    if (mcast_stream_create(base, mcast, local, htons(5350)) < 0)
        return -1;
    mcast.s_addr = inet_addr("224.0.0.252");
    if (mcast_stream_create(base, mcast, local, htons(5252)) < 0)
        return -1;

    event_base_dispatch(base);
    return 0;
}
*/
