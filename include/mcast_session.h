#pragma once

#include <stdint.h>

#include <event2/event.h>
#include <stream_queue.h>

struct mcast_session_config;

struct mcast_session {
    struct mcast_rx* rx;
    struct mcast_tx* tx;

    struct mcast_session* next;
    int64_t id;

    struct stream_queue queue;
};

struct mcast_session* mcast_session_create(struct mcast_session_config* config,
    struct event_base* eventBase, struct stream_buffer* buffer);

struct mcast_session_config* mcast_session_config_free(struct mcast_session_config *cfg);
