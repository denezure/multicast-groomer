#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <arpa/inet.h>
#include <event2/event.h>

#include <stream_buffer.h>

#include <mcast_rx.h>
#include <mcast_tx.h>

void signal_cb(evutil_socket_t sock, short event, void* arg);

int main(int argc, char** argv)
{
    if (argc < 4) {
        printf("Usage: %s <group_address> <port> <interface_address>\n", argv[0]);
        return -1;
    }

    int port = atoi(argv[2]);
    char* groupAddress = argv[1];
    char* interfaceAddress = argv[3];

    int32_t bufferCount = 1 << 12;

    struct in_addr local;

    local.s_addr = inet_addr(interfaceAddress);

    struct event_base* base = event_base_new();
    if (!base) {
        fprintf(stderr, "Error creating event loop.\n");
        return -1;
    }

    struct event* signalEvent = evsignal_new(base, SIGINT, signal_cb, base);

    if (!signalEvent || event_add(signalEvent, NULL) < 0) {
        fprintf(stderr, "Error creating SIGINT handler.\n");
    }

    struct stream_buffer *globalBuffer = stream_buffer_create(bufferCount);

    struct in_addr mcast;

    mcast.s_addr = inet_addr(groupAddress);
    if (mcast_stream_create(base, mcast, local, htons(port)) < 0) {
        printf("Error creating mcast stream.\n");
        return -1;
    }

    event_base_dispatch(base);
    return 0;
}

void signal_cb(evutil_socket_t sock, short events, void* arg)
{
    (void)events;
    (void)sock;
    printf("\nCaught SIGINT. Exiting...\n");

    struct event_base* base = arg;

    struct timeval delay = { 1, 0 };

    event_base_loopexit(base, &delay);
}
