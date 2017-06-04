#pragma once

#include <stdint.h>

struct mcast_session_config
{
    struct mcast_session_config *next;

    char *name;

    char *sourceGroup;
    char *destinationGroup;

    char *sourceInterface;
    char *destinationInterface;

    uint16_t sourcePort;
    uint16_t destinationPort;

    int32_t smoothingWindow;
};

struct mcast_config
{
    char *controlInterface;
    uint16_t controlPort;

    struct mcast_session_config *sessionConfig;
};

// Returns non-zero on failure.
int config_loader_get(const char *filename, struct mcast_config **config);
