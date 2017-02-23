#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <event2/event.h>

#include <config_loader.h>

#include <mcast_session.h>
#include <stream_buffer.h>

#define CONFIG_FILE_NAME "multicast-groomer.yaml"

static void signal_cb(evutil_socket_t sock, short event, void* arg);

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    int32_t bufferCount = 1 << 14;
    int32_t bufferSize = 1400;

    struct event_base* base = event_base_new();
    if (!base) {
        fprintf(stderr, "Error creating event loop.\n");
        return -1;
    }

    struct event* signalEvent = evsignal_new(base, SIGINT, signal_cb, base);

    if (!signalEvent || event_add(signalEvent, NULL) < 0) {
        fprintf(stderr, "Error creating SIGINT handler.\n");
    }

    struct stream_buffer* globalBuffer = stream_buffer_create(bufferCount, bufferSize);

    struct mcast_config* config = NULL;

    if (config_loader_get(CONFIG_FILE_NAME, &config)) {
        fputs("Error loading config.\n", stderr);
        return -1;
    }

    // Initialize the control system first (TODO)

    // Initialize each session

    struct mcast_session_config *sessionConfig = config->sessionConfig;
    while (sessionConfig)
    {
        // Do some stuff.

        sessionConfig = sessionConfig->next;
    }

    event_base_dispatch(base);
    stream_buffer_destroy(globalBuffer);

    return 0;
}

static void signal_cb(evutil_socket_t sock, short events, void* arg)
{
    (void)events;
    (void)sock;
    printf("\nCaught SIGINT. Exiting...\n");

    struct event_base* base = arg;

    struct timeval delay = { 1, 0 };

    event_base_loopexit(base, &delay);
}
