#ifndef OS_PROJ2_PROTOCOL_H
#define OS_PROJ2_PROTOCOL_H

#include "../utils/utils.h"
#include <stdint.h>

/// Variable size
#define MAX_PIPE_NAME (sizeof(char) * 256)
#define MAX_BOX_NAME (sizeof(char) * 32)
#define MAX_MESSAGE (sizeof(char) * 1024)
#define MAX_CODE sizeof(uint8_t)
#define MAX_RETURN_CODE sizeof(int32_t)
#define MAX_N_BOX sizeof(uint64_t)
#define MAX_NUM_BOXES 1000

/// Formatted Requests size
#define MAX_REGISTER_REQUEST (MAX_CODE + MAX_PIPE_NAME + MAX_BOX_NAME + 1)
#define MAX_MESSAGE_REQUEST (MAX_CODE + MAX_MESSAGE + 1)
#define MAX_RESPONSE_REQUEST (MAX_CODE + MAX_RETURN_CODE + MAX_MESSAGE + 1)
#define MAX_LIST_REQUEST (MAX_CODE + MAX_PIPE_NAME + 1)
#define MAX_LIST_RESPONSE_REQUEST                                              \
    ((2 * MAX_CODE) + MAX_BOX_NAME + 3 * MAX_N_BOX + 1)

/// Possible OP_CODE values
#define REQUEST_REGISTER_PUBLISHER 1
#define REQUEST_REGISTER_SUBSCRIBER 2
#define REQUEST_CREATE_BOX 3
#define RESPONSE_CREATE_BOX 4
#define REQUEST_REMOVE_BOX 5
#define RESPONSE_REMOVE_BOX 6
#define REQUEST_LIST_BOX 7
#define RESPONSE_LIST_BOX 8
#define PUBLISHER_MESSAGE 9
#define MESSAGE_SUBSCRIBER 10

/// Create Requests and Responses
char *create_register_request(uint8_t code, char *pipe_name, char *box_name);
char *create_message(uint8_t code, char *message);
char *create_list_request(uint8_t code, char *pipe_name);
char *create_response_request(uint8_t code, int32_t return_code,
                              char *error_message);
char *create_list_response(uint8_t code, int index, uint8_t last);
char *create_zero_response(uint8_t code);

/// Decode Messages
int decode_message(char *dest, char *buffer);

#endif // OS_PROJ2_PROTOCOL_H
