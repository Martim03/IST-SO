#include "../utils/utils.h"

/**
 * Format Request for publisher/subscriber register and box operations
 *
 * Input:
 * - code = operation code
 * - pipe_name = pipe name
 * - box_name = box name
 **/
char *create_register_request(uint8_t code, char *pipe_name, char *box_name) {
    char *output = malloc(MAX_REGISTER_REQUEST);
    if (output == NULL) {
        PANIC("Malloc for request creation failed\n")
    }
    memset(output, 0, MAX_REGISTER_REQUEST);

    // Insert OP_CODE
    memcpy(output, &code, MAX_CODE);
    // Insert Pipe name
    memcpy(output + MAX_CODE, pipe_name, MAX_PIPE_NAME);
    // Insert Box name
    memcpy(output + MAX_CODE + MAX_PIPE_NAME, box_name, MAX_BOX_NAME);

    return output;
}

/**
 * Format Messages for subscriber and publisher
 *
 * Input:
 * - code = operation code
 * - message = message to send
 **/
char *create_message(uint8_t code, char *message) {
    char *output = malloc(MAX_MESSAGE_REQUEST);
    if (output == NULL) {
        PANIC("Malloc for message creation failed\n")
    }
    memset(output, 0, MAX_MESSAGE_REQUEST);

    // Insert OP_CODE
    memcpy(output, &code, MAX_CODE);
    // Insert Message
    memcpy(output + MAX_CODE, message, MAX_MESSAGE);

    return output;
}

/**
 * Format Response for box creation/removal
 *
 * Input:
 * - code = operation code
 * -return_code = return code (0 if succeeded -1 if not)
 * - box_name = box name
 **/
char *create_response_request(uint8_t code, int32_t return_code,
                              char *error_message) {

    char *output = malloc(MAX_RESPONSE_REQUEST);
    if (output == NULL) {
        PANIC("Malloc for response creation failed\n")
    }

    memset(output, 0, MAX_RESPONSE_REQUEST);

    // Insert OP_CODE
    memcpy(output, &code, MAX_CODE);
    // Insert Return code
    memcpy(output + MAX_CODE, &return_code, MAX_RETURN_CODE);
    // Insert Error Message
    memcpy(output + MAX_CODE + MAX_RETURN_CODE, error_message, MAX_MESSAGE);
    return output;
}

/**
 * Format Request for box listing
 *
 * Input:
 * - code = operation code
 * - pipe_name = pipe name
 **/
char *create_list_request(uint8_t code, char *pipe_name) {
    char *output = malloc(MAX_LIST_REQUEST);

    if (output == NULL) {
        PANIC("Malloc for request creation failed\n")
    }

    memset(output, 0, MAX_LIST_REQUEST);

    // Insert OP_CODE
    memcpy(output, &code, MAX_CODE);
    // Insert Pipe name
    memcpy(output + MAX_CODE, pipe_name, MAX_PIPE_NAME);

    return output;
}

/**
 * (Auxiliary function) Format Response for Box Listing
 *
 * Input:
 * - code = operation code
 * - last = 1 if its the last box 0 otherwise
 * - name = box name
 * - size = box size
 * - n_pub = 1 if box has a publisher 0 if not
 * - n_sub = number of subscribers
 **/
void memcpy_list_response(char *output, uint8_t code, uint8_t last,
                          char name[MAX_BOX_NAME], uint64_t size,
                          uint64_t n_pub, uint64_t n_sub) {
    // Insert OP_CODE
    memcpy(output, &code, MAX_CODE);
    // Insert last (1 if it´s the last box // 0 if not)
    memcpy(output + MAX_CODE, &last, MAX_CODE);
    // Insert Box name
    memcpy(output + (2 * MAX_CODE), name, MAX_BOX_NAME);
    // Insert Box size
    memcpy(output + (2 * MAX_CODE) + MAX_BOX_NAME, &size, MAX_N_BOX);
    // Insert n_publisher
    memcpy(output + (2 * MAX_CODE) + MAX_BOX_NAME + MAX_N_BOX, &n_pub,
           MAX_N_BOX);
    // Insert n_subscriber
    memcpy(output + (2 * MAX_CODE) + MAX_BOX_NAME + (MAX_N_BOX * 2), &n_sub,
           MAX_N_BOX);
}

/**
 * Format Response for Box Listing if there are no Boxes
 *
 * Input:
 * - code = operation code
 **/
char *create_zero_response(uint8_t code) {
    char *output = malloc(MAX_LIST_RESPONSE_REQUEST);
    if (output == NULL) {
        PANIC("Malloc for response creation failed\n")
    }
    memset(output, 0, MAX_LIST_RESPONSE_REQUEST);
    uint8_t last = 1;
    char name[MAX_BOX_NAME];
    memset(name, 0, MAX_BOX_NAME);
    uint64_t size = 0;
    uint64_t n_pub = 0;
    uint64_t n_sub = 0;

    memcpy_list_response(output, code, last, name, size, n_pub, n_sub);

    return output;
}

/**
 * Format Response for Box Listing
 *
 * Input:
 * - code = operation code
 *  - index = box index
 *  - last = 1 if its the last box 0 otherwise
 **/
char *create_list_response(uint8_t code, int index, uint8_t last) {
    char *output = malloc(MAX_LIST_RESPONSE_REQUEST);
    if (output == NULL) {
        PANIC("Malloc for response creation failed\n")
    }

    memset(output, 0, MAX_LIST_RESPONSE_REQUEST);

    box_struct *box = box_struct_array[index];

    memcpy_list_response(output, code, last, box->name, box->size,
                         box->n_subscriber, box->n_subscriber);

    return output;
}

/**
 * Decode register request
 *
 * Input:
 * - worker = thread that is handling the operation
 **/
void decode_register(worker_struct *worker) {
    char *buffer = worker->request;

    // Get Pipe name
    memcpy(worker->pipe_name, buffer + MAX_CODE, MAX_PIPE_NAME);
    // Get Box name
    memcpy(worker->box_name, buffer + MAX_CODE + MAX_PIPE_NAME, MAX_BOX_NAME);
}

/**
 * Decode box listing request
 *
 * Input:
 * - worker = thread that is handling the operation
 **/
void decode_list_box(worker_struct *worker) {
    char *buffer = worker->request;

    // Get Pipe name
    memcpy(worker->pipe_name, buffer + MAX_CODE, MAX_PIPE_NAME);
}

/**
 * Handle decoding requests
 *
 * Input:
 * - worker = thread that is handling the operation
 **/
int decode(worker_struct *worker) {
    uint8_t code;
    char *buffer = worker->request;
    memcpy(&code, buffer, MAX_CODE);

    // Handle all cases
    switch (code) {
    case REQUEST_REGISTER_PUBLISHER:
    case REQUEST_REGISTER_SUBSCRIBER:
    case REQUEST_CREATE_BOX:
    case REQUEST_REMOVE_BOX:
        decode_register(worker);
        break;
    case REQUEST_LIST_BOX:
        decode_list_box(worker);
        break;
    default:
        return -1;
    }
    return 0;
}

/**
 * Decode messages
 *
 * Input:
 * - dest = destination buffer (going to store message)
 * - buffer = formatted message
 **/
int decode_message(char *dest, char *buffer) {
    uint8_t code;
    memcpy(&code, buffer, MAX_CODE);
    if (code != PUBLISHER_MESSAGE && code != MESSAGE_SUBSCRIBER) {
        return -1;
    }

    // Get Message
    memcpy(dest, buffer + sizeof(uint8_t), MAX_MESSAGE);
    return 0;
}

/**
 * Decode box listing response
 *
 * Input:
 * - box = destination box (going to store atributes)
 * - buffer = formatter response
 **/
int decode_list_response(box_struct *box, char *buffer) {
    uint8_t code;
    memcpy(&code, buffer, MAX_CODE);
    if (code != RESPONSE_LIST_BOX) {
        return -1;
    }

    char box_name[MAX_BOX_NAME];
    uint64_t box_size;
    uint64_t n_pub;
    uint64_t n_sub;

    // Get Box name
    memcpy(box_name, buffer + (2 * MAX_CODE), MAX_BOX_NAME);
    // Get Box size
    memcpy(&box_size, &buffer[(2 * MAX_CODE) + MAX_BOX_NAME], MAX_N_BOX);
    // Get n_publisher
    memcpy(&n_pub, &buffer[(2 * MAX_CODE) + MAX_BOX_NAME + MAX_N_BOX],
           MAX_N_BOX);
    // Get n_subscriber
    memcpy(&n_sub, &buffer[(2 * MAX_CODE) + MAX_BOX_NAME + (2 * MAX_N_BOX)],
           MAX_N_BOX);

    // Save components in the Box
    strcpy(box->name, box_name);
    box->size = box_size;
    box->n_publisher = n_pub;
    box->n_subscriber = n_sub;
    return 0;
}