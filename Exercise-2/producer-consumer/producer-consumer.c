#include "producer-consumer.h"
#include "stdio.h"
#include <pthread.h>

/**
 * Creates the producer-consumer queue to handle requests.
 */
int pcq_create(pc_queue_t *queue, size_t capacity) {

    queue->pcq_capacity = capacity;
    queue->pcq_current_size = 0;
    queue->pcq_head = 0;
    queue->pcq_tail = 0;

    if (pthread_mutex_init(&queue->pcq_current_size_lock, NULL) != 0 ||
        pthread_mutex_init(&queue->pcq_head_lock, NULL) != 0 ||
        pthread_mutex_init(&queue->pcq_tail_lock, NULL) != 0 ||
        pthread_mutex_init(&queue->pcq_pusher_condvar_lock, NULL) != 0 ||
        pthread_cond_init(&queue->pcq_pusher_condvar, NULL) != 0 ||
        pthread_mutex_init(&queue->pcq_popper_condvar_lock, NULL) != 0 ||
        pthread_cond_init(&queue->pcq_popper_condvar, NULL) != 0)
        return -1;

    return 0;
}

/**
 * Destroys the producer-consumer queue.
 */
int pcq_destroy(pc_queue_t *queue) {
    if (pthread_mutex_destroy(&queue->pcq_current_size_lock) != 0 ||
        pthread_mutex_destroy(&queue->pcq_head_lock) != 0 ||
        pthread_mutex_destroy(&queue->pcq_tail_lock) != 0 ||
        pthread_mutex_destroy(&queue->pcq_pusher_condvar_lock) != 0 ||
        pthread_cond_destroy(&queue->pcq_pusher_condvar) != 0 ||
        pthread_mutex_destroy(&queue->pcq_popper_condvar_lock) != 0 ||
        pthread_cond_destroy(&queue->pcq_popper_condvar) != 0)
        return -1;

    return 0;
}

/**
 * Enqueues a new element at the front of the queue.
 */
int pcq_enqueue(pc_queue_t *queue, void *elem) {
    if (pthread_mutex_lock(&queue->pcq_pusher_condvar_lock) != 0) {
        return -1;
    }

    while (queue->pcq_current_size == queue->pcq_capacity) {
        if (pthread_cond_wait(&queue->pcq_pusher_condvar,
                              &queue->pcq_pusher_condvar_lock) != 0) {
            return -1;
        }
    }

    // If it entered here, it means that there is space in the queue
    if (pthread_mutex_lock(&queue->pcq_head_lock) != 0) {
        return -1;
    }
    if (pthread_mutex_lock(&queue->pcq_current_size_lock) != 0) {
        return -1;
    }

    // Place the element at the front of the queue
    if (queue->pcq_current_size != 0) {
        queue->pcq_tail++;
        // Move everything to the right
        for (size_t i = queue->pcq_tail; i > queue->pcq_head; i--) {
            queue->pcq_buffer[i] = queue->pcq_buffer[i - 1];
        }
    }

    queue->pcq_buffer[queue->pcq_head] = elem;
    queue->pcq_current_size++;

    if (pthread_mutex_unlock(&queue->pcq_current_size_lock) != 0) {
        return -1;
    }
    if (pthread_mutex_unlock(&queue->pcq_head_lock) != 0) {
        return -1;
    }
    if (pthread_mutex_unlock(&queue->pcq_pusher_condvar_lock) != 0) {
        return -1;
    }
    if (pthread_cond_signal(&queue->pcq_popper_condvar) != 0) {
        return -1;
    }

    return 0;
}

/**
 * Dequeues an element from the back of the queue.
 */
void *pcq_dequeue(pc_queue_t *queue) {
    void *elem;

    if (pthread_mutex_lock(&queue->pcq_popper_condvar_lock) != 0) {
        return NULL;
    }

    while (queue->pcq_current_size == 0) {
        if (pthread_cond_wait(&queue->pcq_popper_condvar,
                              &queue->pcq_popper_condvar_lock) != 0) {
            return NULL;
        }
    }

    if (pthread_mutex_lock(&queue->pcq_tail_lock) != 0) {
        return NULL;
    }
    if (pthread_mutex_lock(&queue->pcq_current_size_lock) != 0) {
        return NULL;
    }

    // Get the element at the back of the queue
    elem = queue->pcq_buffer[queue->pcq_tail];
    queue->pcq_current_size--;
    if (queue->pcq_current_size != 0) {
        queue->pcq_tail--;
    }

    if (pthread_mutex_unlock(&queue->pcq_current_size_lock) != 0) {
        return NULL;
    }
    if (pthread_mutex_unlock(&queue->pcq_tail_lock) != 0) {
        return NULL;
    }
    if (pthread_mutex_unlock(&queue->pcq_popper_condvar_lock) != 0) {
        return NULL;
    }
    if (pthread_cond_signal(&queue->pcq_pusher_condvar) != 0) {
        return NULL;
    }
    return elem;
}
