#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <event2/event.h>

#include <config_loader.h>

#include <mcast_session.h>
#include <stream_buffer.h>

#ifdef WIN32

#include <winsock2.h>

#endif

#define CONFIG_FILE_NAME "multicast-groomer.yaml"

static void signal_cb(evutil_socket_t sock, short event, void* arg);

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

#ifdef WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);    
    if (err != 0)
    {
        printf("Error starting Winsock.\n");
        return -1;
    }
#endif

    int32_t bufferCount = 1 << 14;
    int32_t bufferSize = 1600;

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

    struct mcast_session *session = NULL;

    if (config) {
        // Initialize each session
        struct mcast_session_config* sessionConfig = config->sessionConfig;
        while (sessionConfig) {
            printf("Session:\n");
            printf("  Name: %s\n", sessionConfig->name);
            printf("  Source: %s:%d\n", sessionConfig->sourceGroup, sessionConfig->sourcePort);
            printf("  Dest: %s:%d\n", sessionConfig->destinationGroup, sessionConfig->destinationPort);
            printf("  Window: %d microseconds\n", sessionConfig->smoothingWindow);

            // Append to session linked list
            struct mcast_session* newSession = mcast_session_create(sessionConfig, base, globalBuffer);
            newSession->next = session;
            session = newSession;

            sessionConfig = mcast_session_config_free(sessionConfig);
        }

        if (config->controlInterface) {
            free(config->controlInterface);
        }
        free(config);
    }

    event_base_dispatch(base);

    // Free session linked list
    while (session)
    {
        struct mcast_session *next = session->next;
        free(session);
        session = next;
    }

    stream_buffer_destroy(globalBuffer);

#ifdef WIN32
    WSACleanup();
#endif

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
