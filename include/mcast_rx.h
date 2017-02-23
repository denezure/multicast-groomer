#pragma once

#include <event2/event.h>

#include <stream_queue.h>

struct mcast_rx {
    evutil_socket_t sock;
    struct in_addr group;
    uint16_t port;

    struct stream_queue* queue;

    struct event* read_event;
};

struct mcast_rx* mcast_rx_create(struct event_base* eventBase, const char* groupAddr,
    const char* localAddr, uint16_t mcast_port, struct stream_queue* queue);
