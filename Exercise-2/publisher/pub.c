#include "../utils/utils.h"
#include "logging.h"
char *pipe_name;

/**
 * Signal handler for SIGPIPE
 *
 * If the pipe is broken (for example when register request is denied),
 * exits the program.
 *
 */
void sigpipe_handler(int signo) {
    unlink(pipe_name);
    if (signo == SIGPIPE) {
        exit(0);
    }
    perror("Signal error, it was not SIGPIPE");
    exit(1);
}

/**
 * Publisher Client Loop
 *
 * Reads messages separated by /n from stdin and sends them to the mbroker,
 * one at a time.
 *
 * Returns 0 if EOF received, else -1.
 */
int writing_loop(int pipe_fd) {

    while (1) {

        // Read from stdin
        char buffer[1024];
        if (!fgets(buffer, sizeof(buffer), stdin)) {
            return 0;
        }

        // If user didn't write anything, ignore
        if (buffer[0] == '\n') {
            continue;
        }

        // Change \n to \0
        buffer[strlen(buffer) - 1] = '\0';

        // Create request
        char *request;
        request = create_message(PUBLISHER_MESSAGE, buffer);

        // Write the request to the pipe
        if (write(pipe_fd, request, MAX_MESSAGE_REQUEST) == -1) {
            free(request);
            return -1;
        }
        free(request);
    }
}

/**
 * Publisher Client
 *
 * Sends a publisher request to the mbroker and then
 * enters the Publisher Client Loop.
 *
 *
 * Publisher only knows if it was accepted after trying to write in terminal,
 * if it was not accepted the session will then automatically close.
 * -----------------------------------------
 * Trying to write more than 1024  bytes (file limit) on a box with result in a
 * truncation of text, meaning it will only write until it reaches 1024 and
 * ignore the rest.
 *
 * Returns 0 if EOF received, else -1.
 */
int main(int argc, char **argv) {
    // If num of arguments is wrong
    if (argc != 4) {
        fprintf(stderr, "usage: pub <register_pipe> <pipe_name> <box_name>\n");
        return -1;
    }

    char *register_pipe_name = argv[1];
    pipe_name = argv[2];
    char *box_name = argv[3];

    // Format the request protocol for publisher
    char *request = create_register_request(REQUEST_REGISTER_PUBLISHER,
                                            pipe_name, box_name);
    if (mkfifo(pipe_name, 0640) == -1) {
        free(request);
        return -1;
    }

    signal(SIGPIPE, sigpipe_handler);

    // Send the request to the register pipe
    if (send_request(request, register_pipe_name, MAX_REGISTER_REQUEST) == -1) {
        free(request);
        unlink(pipe_name);
        return -1;
    }
    free(request);

    // Open the pipe to start write loop
    int pipe_fd = open(pipe_name, O_WRONLY);
    if (pipe_fd == -1) {
        unlink(pipe_name);
        return -1;
    }

    // Read from stdin and send publisher requests
    if (writing_loop(pipe_fd) == -1) {
        unlink(pipe_name);
        return -1;
    }

    unlink(pipe_name);
    return 0;
}
