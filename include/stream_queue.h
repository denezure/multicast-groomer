#pragma once

#include <stream_buffer.h>

struct stream_queue
{
    // Shared between multiple queues
    struct stream_buffer *buffer;

    struct packet_node *head;
    struct packet_node *tail;

    int32_t length;

    int32_t packetsThisInterval;
};

struct stream_queue stream_queue_create(struct stream_buffer *buf);
void stream_queue_destroy(struct stream_queue *queue);

// Returns 0 when there is nothing to pull 
// Otherwise, *node will point to the node consumed
int32_t stream_queue_try_pull(struct stream_queue *queue, struct packet_node **node);

// Adds node to the queue head
void stream_queue_push(struct stream_queue *queue, struct packet_node *node);
