#pragma once

#include <stdint.h>

struct packet_node {
    struct packet_node *next;

    // Written to when a packet is read.
    // Buffer size is constant.
    int32_t len;
    uint8_t buf[];
};

struct stream_buffer {
    void* buffer;
    struct packet_node* freeList;
    int32_t bufferSize;
    int32_t _packetSize;
};

struct stream_buffer* stream_buffer_create(int32_t bufferCount, int32_t bufferSize);
void stream_buffer_destroy(struct stream_buffer* buf);

struct packet_node* stream_buffer_acquire_packet(struct stream_buffer* buf);
void stream_buffer_release_packet(struct stream_buffer* buf, struct packet_node* node);
