#include <stdlib.h>

#include <stream_queue.h>

struct stream_queue stream_queue_create(struct stream_buffer* buf)
{
    return (struct stream_queue){ buf, NULL, NULL, 0 };
}

void stream_queue_destroy(struct stream_queue* queue)
{
    if (!queue)
        return;

    // Empty all elements from the queue back into the free list of
    // the buffer

    struct stream_buffer* buff = queue->buffer;

    struct packet_node* node = queue->tail;
    while (node) {
        struct packet_node* next = node->next;

        stream_buffer_release_packet(buff, node);

        node = next;
    }

    queue->head = NULL;
    queue->tail = NULL;
    queue->buffer = NULL;
}

int32_t stream_queue_try_pull(struct stream_queue* queue, struct packet_node** node)
{
    if (!queue || !node || !*node)
        return 0;

    if (queue->tail) {
        *node = queue->tail;
        queue->tail = queue->tail->next;

        (*node)->next = NULL;

        --queue->length;

        return 1;
    }

    return 0;
}

void stream_queue_push(struct stream_queue* queue, struct packet_node* node)
{
    if (!queue || !node)
        return;

    node->next = NULL;

    if (!queue->tail) {
        // tail being NULL means the whole queue is empty
        queue->tail = node;
        queue->head = node;
    } else {
        queue->head->next = node;
        queue->head = node;
    }

    ++queue->length;
}

int32_t stream_queue_length(struct stream_queue *queue)
{
    return queue->length;
}
