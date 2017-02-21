#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <event2/event.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>

#include <inttypes.h>
#include <string.h>
#include <unistd.h>

#include <stream_buffer.h>
#include <stream_queue.h>

void read_cb(evutil_socket_t fd, short event, void* arg);

struct rx_conn {
    struct in_addr mcast_ip;
    uint16_t mcast_port;
    uint16_t buffer_usage;

    struct stream_queue* queue;

    struct event* read_event;
};

struct rx_conn* alloc_rx_conn(struct event_base* base, evutil_socket_t fd, struct stream_queue* queue)
{
    printf("Allocating mcast stream.\n");
    struct rx_conn* state = malloc(sizeof(*state));
    if (!state)
        return NULL;

    state->read_event = event_new(base, fd, EV_READ | EV_PERSIST, read_cb, state);
    state->buffer_usage = 0;
    state->queue = queue;

    if (!state->read_event) {
        free(state);
        return NULL;
    }

    return state;
}

void free_rx_conn(struct rx_conn* state)
{
    printf("Freeing mcast stream.\n");
    stream_queue_destroy(state->queue);
    event_free(state->read_event);
    free(state);
}

int mcast_stream_create(struct event_base* base, struct in_addr mcast,
    struct in_addr local, uint16_t mcast_port, struct stream_buffer* buffer)
{
    printf("Creating mcast socket...\n");
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

    struct stream_queue* queue = calloc(1, sizeof(*queue));
    queue->buffer = buffer;

    struct rx_conn* state = alloc_rx_conn(base, nsock, queue);
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
    struct rx_conn* state = arg;

    struct sockaddr_in src_addr;
    socklen_t src_addr_len;

    ssize_t result = 0;
    struct packet_node* packet = stream_buffer_get_free(state->queue->buffer);

    if (packet) {
        state->buffer_usage++;
        result = recvfrom(fd, packet->buf, sizeof(packet->buf), 0, (struct sockaddr*)&src_addr, &src_addr_len);
        packet->len = result;

        printf("Buffer usage: %u\n", state->buffer_usage);

        // For debugging, send the buffer right back.
        stream_buffer_release_packet(state->queue->buffer, packet);
    }

    if (result > 0) {
        return;
    }

    if (result == 0) {
        // The sender has closed? This shouldn't happen.
        fprintf(stderr, "recvfrom() returned 0. Sender closed?\n");
    } else if (result < 0) {
        int error = evutil_socket_geterror(fd);
        fprintf(stderr, "recvfrom() error: %s.\n", evutil_socket_error_to_string(error));

        if (error == EVUTIL_EAI_AGAIN) {
            return;
        }
    }
    free_rx_conn(state);
}

