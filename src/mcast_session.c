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
