#include "../utils/utils.h"
#include <strings.h>

static void print_usage() {
    fprintf(stderr,
            "usage: \n"
            "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
            "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
            "   manager <register_pipe_name> <pipe_name> list\n");
}

/**
 * Sorting function for boxes
 *
 * Input:
 * - v1: box1
 * - v2: box2
 * Returns 1 if Box1 > Box2, -1 if Box2 < Box1, 0 if Box1 == Box2.
 **/
int box_comp(const void *v1, const void *v2) {
    box_struct *b1 = *(box_struct **)v1;
    box_struct *b2 = *(box_struct **)v2;

    char *c1 = b1->name;
    char *c2 = b2->name;
    if (strcasecmp(c1, c2) > 0)
        return 1;
    if (strcasecmp(c1, c2) < 0)
        return -1;

    return 0;
}

/**
 * Receives response from server and handles it (create/remove operations)
 *
 * Input:
 * - fifo_path = pipe name
 **/
int get_response(char *fifo_path) {

    // Open pipe
    int fd = open(fifo_path, O_RDONLY);
    if (fd == -1) {
        return -1;
    }

    // Read Response from pipe
    char buffer[MAX_RESPONSE_REQUEST];
    ssize_t r;
    r = read(fd, buffer, MAX_RESPONSE_REQUEST);

    // Read failed
    if (r == -1) {
        return -1;
    }
    // Pipe closed
    if (r == 0) {
        return -1;
    }

    // Decode Response
    if ((uint32_t)buffer[1] == -1) {
        // Received error response
        char er_message[MAX_MESSAGE];
        strcpy(er_message, buffer + MAX_CODE + MAX_RETURN_CODE);
        fprintf(stdout, "ERROR %s\n", er_message);
        return -1;
    } else {
        // Creation Succeeded
        fprintf(stdout, "OK\n");
    }

    return 0;
}

/**
 * Receives response from server and handles it (list operation)
 *
 * Input:
 * - fifo_path = pipe name
 **/
int get_list_response(char *fifo_path) {

    // Open pipe
    int fd = open(fifo_path, O_RDONLY);
    if (fd == -1) {
        return -1;
    }

    box_struct *array_to_sort[MAX_NUM_BOXES];
    char buffer[MAX_LIST_RESPONSE_REQUEST];
    ssize_t r;
    int numboxes = 0;
    int last = 0;

    // While there are boxes to read
    while (last != 1) {
        memset(buffer, 0, MAX_LIST_RESPONSE_REQUEST);

        // Read Box entry
        r = read(fd, buffer, MAX_LIST_RESPONSE_REQUEST);
        if (r <= 0) {
            return -1;
        }
        memcpy(&last, buffer + MAX_CODE, MAX_CODE);

        // Allocates space for Box
        box_struct *box = malloc(sizeof(box_struct));
        if (box == NULL) {
            PANIC("Failed to allocate memory for box_struct, %s\n",
                  strerror(errno));
        }

        // Decodes Response
        decode_list_response(box, buffer);

        // Checks if Box Name is empty
        char empty[MAX_BOX_NAME];
        memset(empty, 0, MAX_BOX_NAME);
        if (last == 1 && strcmp(empty, box->name) == 0)
            break;

        array_to_sort[numboxes] = box;
        numboxes++;

    }
    if (numboxes == 0) {
        // If there are no Boxes
        fprintf(stdout, "NO BOXES FOUND\n");
    } else {

        // Sorts array
        qsort(array_to_sort, (size_t)numboxes, sizeof(box_struct *), box_comp);
        for (int i = 0; i < numboxes; i++) {
            fprintf(stdout, "%s %zu %zu %zu\n", array_to_sort[i]->name,
                    array_to_sort[i]->size, array_to_sort[i]->n_publisher,
                    array_to_sort[i]->n_subscriber);
            free(array_to_sort[i]);
        }
    }
    return 0;
}

/**
 * Removing a box with connected subscribers automatically closes said
 *subscribers sessions
 *-------------------------------------
 * Trying to remove a box with a connected publisher will result in an error
 * **/
int main(int argc, char **argv) {
    // If num of arguments is wrong
    if (argc != 5 && argc != 4) {
        print_usage();
        return -1;
    }

    char register_pipename[MAX_PIPE_NAME];
    strcpy(register_pipename, argv[1]);
    char pipe_name[MAX_PIPE_NAME];
    strcpy(pipe_name, argv[2]);
    char *command = argv[3];

    if (mkfifo(pipe_name, 0640) == -1) {
        return -1;
    }

    if (argc == 4) {
        // Handle List command
        if (strcmp(command, "list") != 0) {
            print_usage();
            unlink(pipe_name);
            return -1;
        }
        char *request = create_list_request(REQUEST_LIST_BOX, pipe_name);
        // Send Request
        if (send_request(request, register_pipename, MAX_LIST_REQUEST) == -1) {
            unlink(pipe_name);
            return -1;
        }
        free(request);
        // Handle Response
        if (get_list_response(pipe_name) == -1) {
            unlink(pipe_name);
            return -1;
        }
        unlink(pipe_name);
        return 0;
    }

    char box_name[MAX_BOX_NAME];
    strcpy(box_name, argv[4]);

    // Handle Create command
    if (strcmp(command, "create") == 0) {
        char *request =
            create_register_request(REQUEST_CREATE_BOX, pipe_name, box_name);
        // Send Request
        if (send_request(request, register_pipename, MAX_REGISTER_REQUEST) ==
            -1) {
            unlink(pipe_name);
            return -1;
        }
        free(request);
        // Handle Response
        if (get_response(pipe_name) == -1) {
            unlink(pipe_name);
            return -1;
        }
        unlink(pipe_name);
        return 0;
        // Handle Remove command
    } else if (strcmp(command, "remove") == 0) {
        char *request =
            create_register_request(REQUEST_REMOVE_BOX, pipe_name, box_name);
        // Send Request
        if (send_request(request, register_pipename, MAX_REGISTER_REQUEST) ==
            -1) {
            unlink(pipe_name);
            return -1;
        }
        free(request);
        // Handle Response
        if (get_response(pipe_name) == -1) {
            unlink(pipe_name);
            return -1;
        }
        unlink(pipe_name);
        return 0;
    }

    unlink(pipe_name);
    print_usage();
    return -1;
}
