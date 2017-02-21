#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define DUMP_LOCATION fprintf(stderr, "At %s:%d: ", __FILE__, __LINE__)

struct net_rx {
    int sock;
};

void* net_rx_create(char* interface, int port)
{
    int status;
    struct addrinfo hints;
    struct addrinfo* servinfo;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    char port_str[20];
    snprintf(port_str, 20, "%d", port);

    if ((status = getaddrinfo(interface, port_str, &hints, &servinfo)) != 0) {
        DUMP_LOCATION;
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));

        return NULL;
    }

    int s = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    freeaddrinfo(servinfo);

    if (s == -1) {
        int err = errno;
        DUMP_LOCATION;
        fprintf(stderr, "Socket creation error: %d\n", err);

        return NULL;
    }

    struct net_rx* netrx = malloc(sizeof(*netrx));

    if (netrx == NULL) {
        DUMP_LOCATION;
        fprintf(stderr, "Allocation failed!\n");
        close(s);
        return NULL;
    }

    netrx->sock = s;

    return netrx;
}

void net_rx_destroy(struct net_rx* netrx)
{
    if (netrx != NULL) {
        close(netrx->sock);
        netrx->sock = -1;

        free(netrx);
    }
}

void net_rx_add(struct net_rx* netrx, char* group)
{
    int status = 0;

    struct addrinfo hints;
    struct addrinfo* servinfo;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(group, NULL, &hints, &servinfo)) != 0) {
        DUMP_LOCATION;
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));

        return;
    }

    struct ip_mreq mreq = {};
    mreq.imr_multiaddr = ((struct sockaddr_in*)servinfo->ai_addr)->sin_addr;

    freeaddrinfo(servinfo);

    if ((status = setsockopt(netrx->sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(struct ip_mreq))) != 0) {
        DUMP_LOCATION;
        fprintf(stderr, "Error joining group {%s}", group);
    }
}
