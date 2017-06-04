#include <stdlib.h>

#ifndef WIN32

#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#else

#include <winsock2.h>
#include <ws2tcpip.h>

#endif

#include <mcast_rx.h>

static int mcast_rx_config_socket(evutil_socket_t* sock, const struct in_addr* localAddr,
    const struct in_addr* groupAddr, uint16_t portNetworkOrder);

static void read_cb(evutil_socket_t fd, short events, void* arg);

struct mcast_rx* mcast_rx_create(struct event_base* eventBase, const char* groupAddr,
    const char* localAddr, uint16_t port, struct stream_queue* queue)
{
    struct in_addr local = {.s_addr = inet_addr(localAddr) };
    struct in_addr group = {.s_addr = inet_addr(groupAddr) };

    evutil_socket_t sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (mcast_rx_config_socket(&sock, &local, &group, htons(port)) < 0) {
        fprintf(stderr, "Failed to configure socket\n");

        evutil_closesocket(sock);
        return NULL;
    }

    struct mcast_rx* rx = calloc(1, sizeof(*rx));
    if (!rx) {
        fprintf(stderr, "Socket state allocation failed\n");

        evutil_closesocket(sock);
        return NULL;
    }

    rx->queue = queue;

    rx->sock = sock;
    rx->group = group;
    rx->port = port;

    rx->read_event = event_new(eventBase, sock, EV_READ | EV_PERSIST, read_cb, rx);

    if (!rx->read_event) {
        fprintf(stderr, "Socket event creation failed\n");

        free(rx);
        evutil_closesocket(sock);
        return NULL;
    }

    // TODO: Add event for exiting

    event_add(rx->read_event, NULL);
    return rx;
}

static int mcast_rx_config_socket(evutil_socket_t* sock, const struct in_addr* localAddr,
    const struct in_addr* groupAddr, uint16_t portNetworkOrder)
{
    evutil_make_socket_nonblocking(*sock);

    evutil_make_listen_socket_reuseable(*sock);

    {
        struct sockaddr_in sin;

        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_port = portNetworkOrder;

        if (bind(*sock, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
            fprintf(stderr, "bind(): failed.\n");
            return -1;
        }
    }

#ifndef WIN32
    int loop = 1;
    if (setsockopt(*sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0) {
        fprintf(stderr, "setsockopt(): loop failed.\n");
        return -1;
    }
#endif

    struct ip_mreq mreq = {.imr_multiaddr = *groupAddr, .imr_interface = *localAddr };

#ifndef WIN32
    if (setsockopt(*sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
#else
    if (setsockopt(*sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq)) < 0) {
#endif
        fprintf(stderr, "setsockopt(): add membership failed.\n");
        return -1;
    }

    return 0;
}

static void read_cb(evutil_socket_t fd, short events, void* arg)
{
    (void)events;
    struct mcast_rx* state = arg;

    struct sockaddr_in src_addr;
    ev_socklen_t src_addr_len;

    ssize_t result = 0;
    struct packet_node* packet = stream_buffer_acquire_packet(state->queue->buffer);

    if (packet) {
        result = recvfrom(fd, packet->buf, state->queue->buffer->bufferSize, 0, (struct sockaddr*)&src_addr, &src_addr_len);
    }

    if (result > 0) {
        packet->len = result;
        // Also increments the packets per interval variable
        stream_queue_push(state->queue, packet);
        return;
    }

    stream_buffer_release_packet(state->queue->buffer, packet);

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

    event_free(state->read_event);
    stream_queue_destroy(state->queue);
    free(state);
}
