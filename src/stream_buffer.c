#include <stream_buffer.h>

#include <stdlib.h>
#include <string.h>

#define PACKET_BY_INDEX(nodes, i, packetSize) \
    ((struct packet_node*)((uint8_t*)(nodes) + (i) * (packetSize)))

struct stream_buffer* stream_buffer_create(int32_t bufferCount, int32_t bufferSize)
{
    int32_t packetSize = sizeof(struct packet_node) + sizeof(uint8_t) * bufferSize;
    struct packet_node* nodes = calloc(bufferCount, packetSize);

    struct stream_buffer* buf = calloc(1, sizeof(*buf));

    // Make into a circular list
    // Containing all buffers initially
    for (int32_t i = 0; i < bufferCount - 1; ++i) {
        PACKET_BY_INDEX(nodes, i, packetSize)->next = PACKET_BY_INDEX(nodes, i + 1, packetSize);
        PACKET_BY_INDEX(nodes, i, packetSize)->len = 0;
    }

    // Loop last packet back to first
    PACKET_BY_INDEX(nodes, bufferCount - 1, packetSize)->next = nodes;
    PACKET_BY_INDEX(nodes, bufferCount - 1, packetSize)->len = 0;

    buf->buffer = nodes;
    buf->freeList = nodes;
    buf->bufferSize = bufferSize;
    buf->_packetSize = packetSize;

    return buf;
}

void stream_buffer_destroy(struct stream_buffer* buf)
{
    if (buf) {
        if (buf->buffer)
            free(buf->buffer);
        free(buf);
    }
}

struct packet_node* stream_buffer_acquire_packet(struct stream_buffer* buf)
{
    struct packet_node* packet = NULL;

    if (buf && buf->freeList) {
        packet = buf->freeList;
        buf->freeList = packet->next;

        packet->next = NULL;
    }

    return packet;
}

void stream_buffer_release_packet(struct stream_buffer* buf, struct packet_node* node)
{
    node->next = buf->freeList;

    buf->freeList = node;
}
