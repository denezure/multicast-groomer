#include <mcast_tx.h>

#include <stdlib.h>

#ifndef WIN32

#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) < (y) ? (y) : (x))

#else

#include <winsock2.h>
#include <ws2tcpip.h>

#endif

static struct timeval* common_timeout = NULL;

static void timer_cb(evutil_socket_t sock, short events, void* arg);

static int mcast_tx_config_socket(evutil_socket_t* sock, struct in_addr* local);

struct mcast_tx* mcast_tx_create(struct event_base* eventBase, const char* groupAddr,
    const char* localAddr, uint16_t port, struct stream_queue* queue,
    int32_t microsecWindow)
{
    struct in_addr local = {.s_addr = inet_addr(localAddr) };
    struct in_addr group = {.s_addr = inet_addr(groupAddr) };

    // Create socket
    evutil_socket_t sock = socket(AF_INET, SOCK_DGRAM, 0);

    // Bind to local interface & port
    if (mcast_tx_config_socket(&sock, &local) < 0) {
        fprintf(stderr, "Failed to create TX socket.\n");

        evutil_closesocket(sock);
        return NULL;
    }

    // Averaging period for packet rate
    const int32_t microsecInterval = 150;

    // Number of bins that will hold packets per interval
    const int32_t numBins = microsecWindow / microsecInterval;

    // Create mcast_tx holding all this data
    struct mcast_tx* tx = calloc(1, sizeof(*tx) + numBins * sizeof(tx->packetBins[0]));
    if (!tx) {
        fprintf(stderr, "Failed to allocate mcast_tx\n");

        evutil_closesocket(sock);
        return NULL;
    }

    tx->tickPeriod = microsecInterval;

    tx->windowPacketTotal = 0;
    tx->windowTimeInterval = microsecWindow;
    tx->windowLength = numBins;

    tx->sock = sock;
    tx->group = group;
    tx->port = port;
    tx->queue = queue;

    // Set repeating timer event
    tx->timer_event = event_new(eventBase, -1, EV_PERSIST, timer_cb, tx);

    if (!tx->timer_event) {
        fprintf(stderr, "Failed to create TX event\n");

        free(tx);
        evutil_closesocket(sock);
        return NULL;
    }

    if (!common_timeout) {
        struct timeval timeout = { 0, microsecInterval };
        common_timeout = (struct timeval*)event_base_init_common_timeout(eventBase, &timeout);
    }

    event_add(tx->timer_event, common_timeout);

    return tx;
}

static int mcast_tx_config_socket(evutil_socket_t* sock, struct in_addr* local)
{
    struct sockaddr_in sin;

    sin.sin_family = AF_INET;
    sin.sin_addr = *local;
    sin.sin_port = 0;

    if (bind(*sock, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
        fprintf(stderr, "bind(): failed.\n");
        return -1;
    }

    return 0;
}

static void timer_cb(evutil_socket_t sock, short events, void* arg)
{
    (void)sock;
    (void)events;

    struct mcast_tx* tx = arg;

    struct stream_queue* queue = tx->queue;
    int32_t windowWriteIndex = tx->windowWriteIndex;
    int32_t packetsThisInterval = queue->packetsThisInterval;

    // Set the newly found packet count
    tx->packetBins[tx->windowWriteIndex] = packetsThisInterval;

    // Increment the write index
    windowWriteIndex = (windowWriteIndex + 1) % tx->windowLength;

    // Subtract the packet count that fell off the window
    // Add the packet count that was just found
    int32_t windowPacketTotal = tx->windowPacketTotal + packetsThisInterval - tx->packetBins[windowWriteIndex];
    tx->windowPacketTotal = windowPacketTotal;

    // Reset the packets per tick counter
    // This is incremented by the RX system
    queue->packetsThisInterval = 0;
    tx->windowWriteIndex = windowWriteIndex;

    // Calculate how many packets we want to send this tick
    // Cannot be greater than the total number of packets in the queue
    // Should be equal to windowPacketTotal / windowPeriod otherwise
    int32_t packetPerTickAvg = (windowPacketTotal * tx->tickPeriod) / tx->windowTimeInterval;
    int32_t desiredPackets = min(max(1, packetPerTickAvg), queue->length);

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(tx->port),
        .sin_addr = tx->group
    };

    socklen_t sz = sizeof(addr);

    struct packet_node* packet = NULL;
    while ((desiredPackets-- > 0) && stream_queue_try_pull(queue, &packet) > 0) {
        ssize_t result = sendto(tx->sock, packet->buf, packet->len, 0, (struct sockaddr*)&addr, sz);

        printf("TX!\n");

        //printf("  Sent packet... address: 0x%p\n", packet);

        // How do we really want to handle this?
        if (result < 0) {
            fprintf(stderr, "TX error\n");
        }

        stream_buffer_release_packet(queue->buffer, packet);
    }
}
