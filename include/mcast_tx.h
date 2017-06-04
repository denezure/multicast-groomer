#pragma once

#include <event2/event.h>

#include <stream_queue.h>

struct mcast_tx {
    struct stream_queue* queue;
    struct event* timer_event;

    evutil_socket_t sock;
    struct in_addr group;
    uint16_t port;

    int32_t tickPeriod;

    int32_t windowPacketTotal;
    int32_t windowTimeInterval;
    int32_t windowWriteIndex;
    int32_t windowLength;
    int16_t packetBins[];
};

struct mcast_tx* mcast_tx_create(struct event_base* eventBase, const char* groupAddr,
    const char* localAddr, uint16_t mcast_port, struct stream_queue* queue,
    int32_t microsecWindow);
