#pragma once

#include <event2/event.h>

#include <stream_queue.h>

int mcast_rx_create(struct event_base* eventBase, const char* groupAddr,
    const char* localAddr, uint16_t mcast_port, struct stream_queue* queue);
