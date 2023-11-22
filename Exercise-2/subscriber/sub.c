#include "../utils/utils.h"
int message_count = 0;
char *pipe_name;
int pipe_fd;
/**
 * Signal handler for SIGINT
 * Unlinks pipe, prints message count and exits
 */
void sig_handler(int signo) {
    close(pipe_fd);
    unlink(pipe_name);
    if (signo == SIGINT) {
        char buffer[MAX_MESSAGE];
        snprintf(buffer, MAX_MESSAGE, "Read %d messages\n", message_count);
        ssize_t w = write(STDOUT_FILENO, buffer, strlen(buffer));
        if (w == -1) {
            exit(1);
        }
        exit(0);
    } else {
        exit(1);
    }
}

/**
 * Subscriber Client Loop
 *
 * Repeatedly reads data from pipe, and prints it to sdout.
 * Returns -1 if error.
 */
int reading_loop() {
    while (1) {
        char buffer[MAX_MESSAGE_REQUEST];
        char message[MAX_MESSAGE];

        ssize_t n = read(pipe_fd, buffer, MAX_MESSAGE_REQUEST);
        if (n == -1) {
            return -1;
        }
        // checks if pipe was closed
        if (n == 0) {
            return -1;
        }

        // Decode message
        if (decode_message(message, buffer) == -1)
            return -1;

        // Write data to stdout
        fprintf(stdout, "%s\n", message);
        message_count++;
    }
}

/**
 * Subscriber Client
 *
 * Sends a request to mbroker, and then enters the loop.
 *
 * If the connection is rejected, pipe is closed.
 *
 * Returns -1 if error.
 */
int main(int argc, char **argv) {

    // If SIGINT is received, close session
    signal(SIGINT, sig_handler);

    // If num of arguments is wrong
    if (argc != 4) {
        fprintf(stderr,
                "usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
        return -1;
    }

    char *register_pipe_name = argv[1];
    pipe_name = argv[2];
    char *box_name = argv[3];

    // Format the request protocol for subscriber
    char *request = create_register_request(REQUEST_REGISTER_SUBSCRIBER,
                                            pipe_name, box_name);
    if (mkfifo(pipe_name, 0640) == -1) {
        free(request);
        return -1;
    }

    // Send the request
    if (send_request(request, register_pipe_name, MAX_REGISTER_REQUEST) == -1) {
        free(request);
        unlink(pipe_name);
        return -1;
    }
    free(request);

    // open the pipe
    pipe_fd = open(pipe_name, O_RDONLY);
    if (pipe_fd == -1) {
        unlink(pipe_name);
        return -1;
    }

    // Read from pipe and write it to stdout
    reading_loop();

    unlink(pipe_name);
    return 0;
}