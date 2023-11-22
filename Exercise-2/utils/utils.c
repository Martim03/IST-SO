#include "utils.h"

int max_sessions;
worker_struct *worker_struct_array;
box_struct *box_struct_array[MAX_NUM_BOXES];

/**
 * Adds "/" to name_box so it can be opened by tfs_open
 *
 * Input:
 * - name = box name
 * - mode = opening mode
 **/
int open_box(char *name, tfs_file_mode_t mode) {

    char file_to_open[MAX_BOX_NAME + 1] = "/";
    strcat(file_to_open, name);

    // Open the file in TFS associated to the box
    int fd = tfs_open(file_to_open, mode);
    if (fd == -1)
        return -1;

    return fd;
}

/**
 * Adds "/" to name_box so it can be unlinked by tfs_unlink
 *
 * Input:
 * - name = box name
 **/
int unlink_box(char *name) {

    char file_to_unlink[MAX_BOX_NAME + 1] = "/";
    strcat(file_to_unlink, name);

    return tfs_unlink(file_to_unlink);
}

/**
 * Free resources in box system
 *
 * Input:
 * - box_array = array of boxes
 **/
int destroy_box_system(box_struct **box_array) {

    // Free all boxes
    for (int i = 0; i < MAX_NUM_BOXES; i++) {

        // unlink boxes
        unlink_box(box_array[i]->name);
        free(box_array[i]->subscribers);

        if(pthread_mutex_destroy(&box_array[i]->mutex) != 0 ||
            pthread_cond_destroy(&box_array[i]->cond) != 0) return -1;

        free(box_array[i]);
    }

    return 0;
}

/**
 * Get total number of currently actives boxes
 **/
int get_num_of_boxes() {
    int num = 0;
    for (int i = 0; i < MAX_NUM_BOXES; i++) {
        if (box_struct_array[i]->free == 0) {
            num++;
        }
    }
    return num;
}

/**
 * Recevies a box name and returns it's index
 *
 * Input:
 * - name = box name
 **/
int get_box(char *name) {
    for (int i = 0; i < MAX_NUM_BOXES; i++) {
        // If box is free, ignore it
        if (box_struct_array[i]->free == 1) {
            continue;
        }
        if (strcmp(name, box_struct_array[i]->name) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * Sends a request (created based on the protocol) through a pipe
 *
 * Input:
 * - request = formatted string
 * - pipe_name = pipe name
 * - size = size of request
 **/
int send_request(char *request, char *pipe_name, size_t size) {

    // Open pipe
    int pipe_fd = open(pipe_name, O_WRONLY);
    if (pipe_fd == -1) {
        return -1;
    }

    // Write to pipe
    if (write(pipe_fd, request, size) == -1) {
        return -1;
    }

    // Close pipe
    if (close(pipe_fd) == -1) {
        return -1;
    }

    return 0;
}

/**
 * Creates a box
 *
 * Input:
 * - worker = thread that is handling the operation
 **/
int create_box(worker_struct *worker) {

    char *box_name = worker->box_name;

    // Check if box is already occupied
    if (get_box(box_name) != -1) {
        return -1;
    }


    // Find free box_struct
    int new_box_idx = -1;
    for (int i = 0; i < MAX_NUM_BOXES; i++) {
        if (box_struct_array[i]->free == 1) {
            new_box_idx = i;
            break;
        }
    }

    // If there are no free boxes
    if (new_box_idx == -1) {
        return -1;
    }

    // Creates the box
    int fd = open_box(box_name, TFS_O_CREAT);
    if (fd == -1) {
        return -1;
    }

    // Closes it
    if (tfs_close(fd) == -1) {
        return -1;
    }

    worker->box_index = new_box_idx;

    // Initializing Box
    box_struct_array[new_box_idx]->free = 0;
    memcpy(box_struct_array[new_box_idx]->name, box_name, MAX_BOX_NAME);

    // Initializing the box attributes
    box_struct_array[new_box_idx]->publisher = -1;
    for (int i = 0; i < max_sessions; i++) {
        box_struct_array[new_box_idx]->subscribers[i] = -1;
    }
    box_struct_array[new_box_idx]->n_publisher = 0;
    box_struct_array[new_box_idx]->n_subscriber = 0;
    box_struct_array[new_box_idx]->size = 0;

    return 0;
}

/**
 * Removes a box
 *
 * Input:
 * - worker = thread that is handling the operation
 **/
int remove_box(worker_struct *worker) {

    // Check if box exists
    int box_idx = get_box(worker->box_name);
    if (box_idx == -1) {
        return -1;
    }

    worker->box_index = box_idx;

    // Check if box has publishers
    if (box_struct_array[box_idx]->n_publisher > 0) {
        return -1;
    }

    // Remove box file from TFS
    if (unlink_box(box_struct_array[box_idx]->name) == -1) {
        return -1;
    }

    // Reset all the box attributes
    box_struct_array[box_idx]->free = 1;
    memset(box_struct_array[box_idx]->name, 0, MAX_BOX_NAME);
    box_struct_array[box_idx]->publisher = -1;
    box_struct_array[box_idx]->n_publisher = 0;
    box_struct_array[box_idx]->n_subscriber = 0;
    box_struct_array[box_idx]->size = 0;

    // Close all subscriber pipes connected to the box
    for (int i = 0; i < max_sessions; i++) {
        if (box_struct_array[box_idx]->subscribers[i] != -1) {
            if (close(box_struct_array[box_idx]->subscribers[i]) == -1) {
                return -1;
            }
        }
    }


    // Signal all subscribers that the box was removed, so they can end session
    if (pthread_cond_broadcast(&box_struct_array[box_idx]->cond) == -1) {
        return -1;
    }

    return 0;
}

/**
 * Returns index of the last box in box_struct_array
 **/
int get_last() {
    int last_index = 0;
    for (int i = 0; i < MAX_NUM_BOXES; i++) {
        if (box_struct_array[i]->free == 0)
            last_index = i;
    }
    return last_index;
}

/**
 * Creates and sends listing box responses
 *
 * Input:
 * - worker = thread that is handling the operation
 **/
int list_box(worker_struct *worker) {

    char *response;
    uint8_t last = 0;

    // If there are no boxes, send a No boxes response
    if (get_num_of_boxes() == 0) {
        // Create a response with no boxes
        response = create_zero_response(RESPONSE_LIST_BOX);
        if (response == NULL)
            return -1;
        if (send_request(response, worker->pipe_name, MAX_LIST_RESPONSE_REQUEST) == -1)
            return -1;
        free(response);
    }

    for (int i = 0; i < MAX_NUM_BOXES; i++) {

        // If box is not occupied, ignore it
        if (box_struct_array[i]->free == 1) {
            continue;
        }

        // If it is the last box, set last element to 1
        if (i == get_last())
            last = 1;

        // Create response to list the current box
        response = create_list_response(RESPONSE_LIST_BOX, i, last);
        if (response == NULL)
            return -1;

        // Send the response to the manager pipe
        if (send_request(response, worker->pipe_name, MAX_LIST_RESPONSE_REQUEST) == -1)
            return -1;

        free(response);
    }
    return 0;
}

/**
 * Registers a publisher
 *
 * Input:
 * - worker = thread that is handling the operation
 **/
int register_publisher(worker_struct *worker) {

    // Check if box exists
    int box_idx = get_box(worker->box_name);
    if (box_idx == -1) {
        return -1;
    }

    worker->box_index = box_idx;

    // Checks if the box already has a publisher
    if (box_struct_array[box_idx]->n_publisher != 0) {
        return -1;
    }

    // Update the box attributes with the new publisher
    box_struct_array[box_idx]->n_publisher++;
    box_struct_array[box_idx]->publisher = worker->index;

    return 0;
}

/**
 * Registers a subscriber
 *
 * Input:
 * - worker = thread that is handling the operation
 **/
int register_subscriber(worker_struct *worker) {

    // Check if box exists
    int box_idx = get_box(worker->box_name);
    if (box_idx == -1) {
        return -1;
    }

    worker->box_index = box_idx;

    // Update the box attributes with the new subscriber
    box_struct_array[box_idx]->n_subscriber++;
    box_struct_array[box_idx]->subscribers[worker->index] = worker->index;

    return 0;
}

/**
 * Handles all requests
 *
 * Input:
 * - worker = thread that is handling the operation
 **/
int handle_request(worker_struct *worker) {

    uint8_t code;
    memcpy(&code, worker->request, MAX_CODE);
    int result = -1;
    char *response;

    // Decode the request and set worker attributes
    if (decode(worker) == -1)
        return -1;

    // Handle all request cases
    switch (code) {
    case REQUEST_CREATE_BOX: // Manager Request: Create Box
        result = create_box(worker);
        response = create_response_request(RESPONSE_CREATE_BOX, result, "Could not create box");
        if (response == NULL)
            return -1;
        if (send_request(response, worker->pipe_name, MAX_RESPONSE_REQUEST) ==
            -1)
            return -1;
        free(response);
        return result;
    case REQUEST_REMOVE_BOX: // Manager Request: Remove Box
        result = remove_box(worker);
        response = create_response_request(RESPONSE_REMOVE_BOX, result,
                                           "Could not remove box");
        if (response == NULL)
            return -1;
        if (send_request(response, worker->pipe_name, MAX_RESPONSE_REQUEST) ==
            -1)
            return -1;
        free(response);
        return result;
    case REQUEST_LIST_BOX: // Manager Request: List Boxes
        result = list_box(worker);
        return result;
    case REQUEST_REGISTER_PUBLISHER: // Publisher Request: Register Publisher
        if (register_publisher(worker) == -1) {
            // If request denied, close the pipe
            close(open(worker->pipe_name, O_RDONLY));
            return -1;
        }
        return 1;
    case REQUEST_REGISTER_SUBSCRIBER: // Subscriber Request: Register Subscriber
        if (register_subscriber(worker) == -1) {
            // If request denied, close the pipe
            close(open(worker->pipe_name, O_WRONLY));
            return -1;
        }
        return 2;
    default:
        return -1;
    }
}

/**
 * Unlocks a mutex
 *
 * Input:
 * - mutex = mutex to be unlocked
 **/
int mutex_unlock(pthread_mutex_t *mutex) {
    if (pthread_mutex_unlock(mutex) != 0) {
        WARN("Failed to unlock mutex: %s", strerror(errno));
        return -1;
    }
    return 0;
}

/**
 * Locks a mutex
 *
 * Input:
 * - mutex = mutex to be locked
 **/
int mutex_lock(pthread_mutex_t *mutex) {
    if (pthread_mutex_lock(mutex) != 0) {
        WARN("Failed to lock mutex: %s", strerror(errno));
        return -1;
    }
    return 0;
}