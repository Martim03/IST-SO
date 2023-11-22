#include "../utils/utils.h"

// Producer-consumer queue
pc_queue_t request_queue;

// Worker Threads
pthread_t *worker_threads;

// Register Pipe, exclusive to the main thread
char *register_pipename;

/**
 * Handles SIGPIPE signal
 */
void sigpipe_handler(int signo) {
    // SIGPIPE handler
    if (signo == SIGPIPE) {
        // Subscriber's pipe was closed
        // There's nothing to do
        // The worker thread of that subscriber handles it
    } else {
        exit(1);
    }
    signal(SIGPIPE, sigpipe_handler);
}

/**
 * Handles SIGINT signal.
 * Exits the program, destroying pcq, boxes, and unlinking all pipes.
 */
void sig_handler(int signo) {
    // SIGINT handler
    if (signo == SIGINT) {
        // Unlink the register pipe
        unlink(register_pipename);

        // Destroy the worker threads, and delete the pipes
        for (int i = 0; i < max_sessions; i++) {
            pthread_cancel(worker_threads[i]);
            close(worker_struct_array[i].pipe_fd);
            unlink(worker_struct_array[i].pipe_name);
        }
        // Destroy the request queue
        if (pcq_destroy(&request_queue) == -1) {
            exit(1);
        }
        // Destroy all boxes
        if (destroy_box_system(box_struct_array) == -1) {
            exit(1);
        }
        // Destroy TFS
        if (tfs_destroy() == -1) {
            exit(1);
        }
        exit(0);
    }
    exit(1);
}

/**
 * Publisher Loop
 *
 * Repeatedly reads from the pipe and publishes the data to the box.
 *
 * Returns 0 if the pipe was closed by the publisher, -1 if an error occurred.
 */
int do_publisher_work(worker_struct *worker) {

    // Open client publisher pipe
    int pipe_fd = open(worker->pipe_name, O_RDONLY);
    if (pipe_fd == -1) {
        return -1;
    }

    // Client pipe
    worker->pipe_fd = pipe_fd;
    box_struct_array[worker->box_index]->publisher = pipe_fd;

    while (1) {

        char buffer[MAX_MESSAGE_REQUEST];
        char message[MAX_MESSAGE];
        memset(buffer, 0, MAX_MESSAGE_REQUEST);
        memset(message, 0, MAX_MESSAGE);

        // Read from pipe
        ssize_t r = read(pipe_fd, buffer, MAX_MESSAGE_REQUEST);
        if (mutex_lock(&box_struct_array[worker->box_index]->mutex) == -1) {
            return -1;
        }
        if (r == -1) { // If read fails
            mutex_unlock(&box_struct_array[worker->box_index]->mutex);
            return -1;
        }
        if (r == 0) { // If pipe is closed
            return mutex_unlock(&box_struct_array[worker->box_index]->mutex);
        }

        // Decode message send by client
        decode_message(message, buffer);

        // Open box for writing
        int fd = open_box(worker->box_name, TFS_O_APPEND);
        if (fd == -1) {
            mutex_unlock(&box_struct_array[worker->box_index]->mutex);
            return -1;
        }

        // Write to the box
        ssize_t bytes;
        if ((bytes = tfs_write(fd, message, strlen(message) + 1)) == -1) {
            mutex_unlock(&box_struct_array[worker->box_index]->mutex);
            return -1;
        }
        // Increment box size accordingly
        box_struct_array[get_box(worker->box_name)]->size += (uint64_t)bytes;

        // Close box
        if (tfs_close(fd) == -1) {
            mutex_unlock(&box_struct_array[worker->box_index]->mutex);
            return -1;
        }

        if (mutex_unlock(&box_struct_array[worker->box_index]->mutex) == -1) {
            return -1;
        }

        // Broadcast write to all Subscribers of that box
        if (pthread_cond_broadcast(
                &box_struct_array[worker->box_index]->cond) == -1) {
            return -1;
        }
    }
}

/**
 * Subscriber Loop
 *
 * Reads all available data from box.
 * Then, repeatedly waits for new data to be published, and reads it when signaled.
 *
 * Returns 0 if the pipe was closed or the box removed, else -1.
 */
int do_subscriber_work(worker_struct *worker) {

    // Open client subscriber pipe
    int pipe_fd = open(worker->pipe_name, O_WRONLY);
    if (pipe_fd == -1) {
        return -1;
    }

    // Client pipe
    worker->pipe_fd = pipe_fd;
    box_struct_array[worker->box_index]->subscribers[worker->index] = pipe_fd;

    // Open box for reading
    int fd = open_box(worker->box_name, 0);
    if (fd == -1) {
        return -1;
    }

    while (1) {

        char buffer[MAX_MESSAGE];
        memset(buffer, 0, MAX_MESSAGE);
        if (mutex_lock(&box_struct_array[worker->box_index]->mutex) == -1) {
            return -1;
        }

        // Read from box, until there's nothing more to read
        ssize_t r;
        while ((r = tfs_read(fd, buffer, MAX_MESSAGE)) == 0) {

            // Check if box has been removed
            if (box_struct_array[worker->box_index]->free == 1) {
                return mutex_unlock(
                    &box_struct_array[worker->box_index]->mutex);
            }

            // Wait for publisher to signal that there's new data
            if (pthread_cond_wait(
                    &box_struct_array[worker->box_index]->cond,
                    &box_struct_array[worker->box_index]->mutex) == -1) {
                return -1;
            }
        }
        if (r == -1) { // If read fails
            mutex_unlock(&box_struct_array[worker->box_index]->mutex);
            return -1;
        }

        // If something was read, create protocol message
        worker->request = create_message(MESSAGE_SUBSCRIBER, buffer);
        if (worker->request == NULL) {
            mutex_unlock(&box_struct_array[worker->box_index]->mutex);
            return -1;
        }

        // Write message to pipe
        ssize_t w = write(pipe_fd, worker->request, MAX_MESSAGE_REQUEST);
        if (w == -1) {
            return mutex_unlock(&box_struct_array[worker->box_index]->mutex);
        }

        // Unlock mutex
        if (mutex_unlock(&box_struct_array[worker->box_index]->mutex) == -1) {
            return -1;
        }
    }
}


/**
 * Resets the Worker struct to its initial state.
 */
void endloop(worker_struct *worker) {

    // Reset the worker thread
    // to handle another request
    memset(worker->box_name, 0, MAX_BOX_NAME);
    memset(worker->pipe_name, 0, MAX_PIPE_NAME);
    worker->request = NULL;
    worker->box_index = -1;
    worker->pipe_fd = -1;
}

/**
 * Worker Thread Loop
 *
 * Waits for a request to be received, then dequeues it and handles it,
 * and waits for more requests.
 *
 * Ends when mbroker is closed.
 */
void *worker_thread_loop(void *worker_index) {

    int worker_id = *((int *)worker_index);
    free(worker_index);

    // Set thread id
    worker_struct *worker = &worker_struct_array[worker_id];
    worker->index = worker_id;

    while (1) {

        // Wait for a request and dequeue it
        worker->request = pcq_dequeue(&request_queue);
        if (worker->request == NULL) {
            WARN("Error on dequeue\n")
        }

        // Handle the registration/manager request
        int h = handle_request(worker);
        if (h == -1) {
            endloop(worker);
            continue;
        }

        // If it was a Publisher Register request
        if (h == 1) {
            if (do_publisher_work(worker) == -1) {
                endloop(worker);
                continue;
            }

            // Update box publisher status
            box_struct_array[worker->box_index]->publisher = -1;
            box_struct_array[worker->box_index]->n_publisher = 0;

        } else if (h == 2) { // If it was a Subscriber Register request

            if (do_subscriber_work(worker) == -1) {
                endloop(worker);
                continue;
            }
            box_struct_array[worker->box_index]->subscribers[worker->index] =
                -1;
            box_struct_array[worker->box_index]->n_subscriber--;
        }

        endloop(worker);
    }
}

/**
 * Initializes mbroker
 */
int initialize_mbroker() {

    signal(SIGINT, sig_handler);
    signal(SIGPIPE, sigpipe_handler);

    // Initialize tfs
    if (tfs_init(NULL) == -1) {
        return -1;
    }

    // create registration pipe
    if (mkfifo(register_pipename, 0640) == -1) {
        return -1;
    }

    // Allocate memory for producer-consumer queue
    request_queue.pcq_buffer = malloc((size_t)max_sessions * 2 * sizeof(void *));
    if (request_queue.pcq_buffer == NULL) {
        PANIC("Malloc for request queue failed\n")
    }

    // Create producer-consumer queue
    if (pcq_create(&request_queue, (size_t)max_sessions * 2) != 0) {
        PANIC("Producer-consumer queue creation failed, %s\n", strerror(errno))
    }

    // Create box struct array
    for (int i = 0; i < MAX_NUM_BOXES; i++) {
        box_struct_array[i] = malloc(sizeof(box_struct));
        if (box_struct_array[i] == NULL) {
            PANIC("Malloc for box struct failed, %s\n", strerror(errno))
        }

        memset(box_struct_array[i]->name, 0, MAX_BOX_NAME);
        box_struct_array[i]->free = 1;
        box_struct_array[i]->publisher = -1;
        box_struct_array[i]->subscribers =
            malloc(sizeof(int) * (size_t)max_sessions);
        for (int j = 0; j < max_sessions; j++) {
            box_struct_array[i]->subscribers[j] = -1;
        }
        box_struct_array[i]->n_subscriber = 0;
        box_struct_array[i]->n_publisher = 0;
        if (pthread_mutex_init(&box_struct_array[i]->mutex, NULL) != 0) {
            PANIC("Mutex init failed\n")
        }
        if (pthread_cond_init(&box_struct_array[i]->cond, NULL) != 0) {
            PANIC("Cond init failed\n")
        }
    }

    // Initialize worker struct array
    worker_struct_array =
        (worker_struct *)malloc((size_t)max_sessions * sizeof(worker_struct));
    if (worker_struct_array == NULL) {
        PANIC("Malloc for worker struct array failed, %s\n", strerror(errno))
    }

    for (int i = 0; i < max_sessions; i++) {
        worker_struct_array[i].request = NULL;
    }

    // allocate worker threads
    worker_threads = malloc(sizeof(pthread_t) * (size_t)max_sessions);
    if (worker_threads == NULL) {
        PANIC("Malloc for worker threads failed, %s\n", strerror(errno))
    }

    for (int i = 0; i < max_sessions; i++) {
        int *p = (int *)malloc(sizeof(int));
        *p = i;
        if (pthread_create(&worker_threads[i], NULL, &worker_thread_loop,
                           (void *)p) != 0) {
            PANIC("pthread_create failed, %s\n", strerror(errno))
        }
    }

    return 0;
}

/**
 * Main thread.
 * Initializes mbroker, then waits for requests.
 * Reads requests from the registration pipe and enqueues them.
 */
int main(int argc, char **argv) {

    if (argc != 3) {
        fprintf(stderr, "usage: mbroker <register_pipename> <max_sessions>\n");
        return -1;
    }
    register_pipename = argv[1];
    max_sessions = atoi(argv[2]);

    if (initialize_mbroker() == -1) {
        return -1;
    }

    // main thread, that assigns requests to producer-consumer queue
    // open the register pipe
    int register_pipe_fd = open(register_pipename, O_RDONLY);
    if (register_pipe_fd == -1) {
        return -1;
    }

    int register_pipe_block_fd = open(register_pipename, O_WRONLY);
    if (register_pipe_block_fd == -1) {
        return -1;
    }

    while (1) {

        // Read Request from the register pipe
        char *request = (char *)malloc(sizeof(char) * MAX_REGISTER_REQUEST);
        if (request == NULL) {
            PANIC("Malloc for buf failed, %s\n", strerror(errno))
        }

        size_t n =
            (size_t)read(register_pipe_fd, request, MAX_REGISTER_REQUEST);
        if (n == -1) {
            return -1;
        }
        if (n == 0) {
            memset(request, 0, MAX_REGISTER_REQUEST);
            continue;
        }

        // Place request on queue
        if (pcq_enqueue(&request_queue, request) == -1) {
            return -1;
        }
    }
}
