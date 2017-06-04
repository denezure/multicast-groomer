#include <mcast_session.h>

#include <config_loader.h>

#include <mcast_rx.h>
#include <mcast_tx.h>
#include <stream_buffer.h>
#include <stream_queue.h>

#include <event2/event.h>

#include <stdlib.h>

static int64_t sessions = 0;

struct mcast_session* mcast_session_create(struct mcast_session_config* config,
    struct event_base* eventBase, struct stream_buffer* buffer)
{
    struct mcast_session* session = calloc(1, sizeof(*session));
    if (session) {
        session->id = sessions++;

        session->queue = stream_queue_create(buffer);

        session->rx = mcast_rx_create(eventBase, config->sourceGroup,
            config->sourceInterface, config->sourcePort, &session->queue);

        session->tx = mcast_tx_create(eventBase, config->destinationGroup, config->destinationInterface,
            config->destinationPort, &session->queue, config->smoothingWindow);
    }

    return session;
}

struct mcast_session_config* mcast_session_config_free(struct mcast_session_config* cfg)
{
    if (cfg) {
        struct mcast_session_config* next = cfg->next;

        int32_t count = 0;
        void* toFree[16];

        if (cfg->name) {
            toFree[count++] = cfg->name;
        }

        if (cfg->sourceGroup) {
            toFree[count++] = cfg->sourceGroup;
        }

        if (cfg->destinationGroup) {
            toFree[count++] = cfg->destinationGroup;
        }

        if (cfg->sourceInterface) {
            toFree[count++] = cfg->sourceInterface;
        }

        if (cfg->destinationInterface) {
            toFree[count++] = cfg->destinationInterface;
        }

        toFree[count++] = cfg;

        for (int32_t i = 0; i < count; ++i) {
            free(toFree[i]);
        }

        return next;
    }

    return NULL;
}
