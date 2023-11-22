#ifndef OS_PROJ2_UTILS_H
#define OS_PROJ2_UTILS_H

#include "../fs/operations.h"
#include "../fs/state.h"
#include "../producer-consumer/producer-consumer.h"
#include "../protocol/protocol.h"
#include "logging.h"
#include "pthread.h"
#include "sys/wait.h"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/// Max Sessions given to mbroker
extern int max_sessions;

/// Worker Struct
typedef struct {
    // Request received
    char *request;
    // Worker index
    int index;
    // box name
    char box_name[MAX_BOX_NAME];
    // box index
    int box_index;

    // pipe name
    char pipe_name[MAX_PIPE_NAME];
    // pipe file descriptor
    int pipe_fd;
} worker_struct;

/// Box Struct
typedef struct {
    // is box free?
    int free;

    // Box name
    char name[MAX_BOX_NAME];
    // Publisher file descriptor
    int publisher;
    // array of connected subscriber file descriptors
    int *subscribers;

    // Box size
    uint64_t size;
    // Has publisher? (1 if yes // 0 if no)
    uint64_t n_publisher;
    // Num of subscribers
    uint64_t n_subscriber;

    pthread_cond_t cond;
    pthread_mutex_t mutex;
} box_struct;

/// Worker threads array
extern worker_struct *worker_struct_array;
/// Array of boxes
extern box_struct *box_struct_array[MAX_NUM_BOXES];


int get_box(char *name);
int get_num_of_boxes();

/// Request Sender/Handler
int send_request(char *request, char *pipe_name, size_t size);
int handle_request(worker_struct *worker);

/// Decoders
int decode(worker_struct *worker);
int decode_list_response(box_struct *box, char *buffer);

/// Requests creator
int create_box(worker_struct *worker);
int remove_box(worker_struct *worker);
int list_box(worker_struct *worker);
int register_publisher(worker_struct *worker);
int register_subscriber(worker_struct *worker);

/// Worker thread functions
int do_publisher_work(worker_struct *worker);
int do_subscriber_work(worker_struct *worker);

/// Box functions
int open_box(char *name, tfs_file_mode_t mode);
int unlink_box(char *name);
int destroy_box_system(box_struct **box_array);

/// Locks
int mutex_lock(pthread_mutex_t *mutex);
int mutex_unlock(pthread_mutex_t *mutex);

#endif // OS_PROJ2_UTILS_H
