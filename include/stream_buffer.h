#pragma once

#include <stdint.h>

#define BUFFER_SIZE 1400

struct packet_node {
    struct packet_node *next;

    int32_t len;
    uint8_t buf[BUFFER_SIZE];
};

struct stream_buffer {
    void* buffer;
    struct packet_node* freeList;
};

struct stream_buffer* stream_buffer_create(int32_t bufferCount);
void stream_buffer_destroy(struct stream_buffer* buf);

struct packet_node* stream_buffer_get_free(struct stream_buffer* buf);
void stream_buffer_release_packet(struct stream_buffer* buf, struct packet_node* node);
