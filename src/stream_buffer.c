#include <stream_buffer.h>

#include <stdlib.h>
#include <string.h>

#define min(x, y) (x) < (y) ? (x) : (y)

/*
 * Consumer end has a pointer-to-pointer-to a packet_node
 * When the *node is NULL, nothing is ready to be sent.
 * Otherwise, advance it and send the packet.
 * Release the packet to the stream_buffer as well.
 */

struct stream_buffer* stream_buffer_create(int32_t bufferCount)
{
    struct packet_node* nodes = calloc(bufferCount, sizeof(*nodes));

    struct stream_buffer* buf = calloc(1, sizeof(*buf));

    // Only the forward list needs to be maintained
    // for the free-list
    nodes[0].next = nodes + 1;

    for (int32_t i = 1; i < bufferCount - 1; ++i) {
        // Assign all buffers to be in the free-list
        nodes[i].next = &nodes[i + 1];
        nodes[i].len = 0;
    }

    nodes[bufferCount - 1].next = nodes + 0;

    buf->freeList = nodes;

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

struct packet_node *stream_buffer_get_free(struct stream_buffer* buf)
{
    struct packet_node *packet = NULL;

    if (buf && buf->freeList)
    {
        packet = buf->freeList;
        buf->freeList = packet->next;

        packet->next = NULL;
    }

    return packet;
}

void stream_buffer_release_packet(struct stream_buffer* buf, struct packet_node *node)
{
    node->next = buf->freeList;

    buf->freeList = node;
}
