#include "../fs/operations.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

uint8_t const file_contents[] = "SO PROJECT!";
char f1[] = "/f1";
char f2[] = "/f2";
char f3[] = "/f3";

void assert_contents_ok(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    uint8_t buffer[sizeof(file_contents)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == sizeof(buffer));
    assert(memcmp(buffer, file_contents, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

void write_contents(char const *path) {
    int f = tfs_open(path, TFS_O_APPEND);
    assert(f != -1);

    assert(tfs_write(f, file_contents, sizeof(file_contents)) ==
           sizeof(file_contents));

    assert(tfs_close(f) != -1);
}

void *th_run01() {
    assert(tfs_sym_link(f1, f2) != -1);

    int f = tfs_open(f2, 0);

    if (f != -1) { // Still hasnt been unlinked by thread 3
        char buffer[50];
        assert(tfs_read(f, buffer, sizeof(buffer)) != -1);
        assert(tfs_close(f) != -1);
    }

    return 0;
}

void *th_run02() {
    assert(tfs_link(f1, f3) != -1);

    write_contents(f3);

    return 0;
}

void *th_run03() {
    int f = tfs_open(f1, 0);
    assert(f != -1);

    char buffer[50];
    assert(tfs_read(f, buffer, sizeof(buffer)) != -1);

    assert(tfs_close(f) != -1);

    assert(tfs_unlink(f1) != -1);
    return 0;
}

void create(char *f) {
    int fd = tfs_open(f, TFS_O_CREAT);
    assert(fd != -1);
    assert(tfs_close(fd) != -1);
}

int main() {
    assert(tfs_init(NULL) != -1);

    create(f1);

    // Creates threads
    pthread_t t1, t2, t3;

    // run1 - Creates a soft_link and tries to read through it
    assert(pthread_create(&t1, NULL, th_run01, NULL) == 0);
    // run2 - Creates a hard_link and writes through it
    assert(pthread_create(&t2, NULL, th_run02, NULL) == 0);
    //run3 - Reads a file and unlinks the previously created soft_link
    assert(pthread_create(&t3, NULL, th_run03, NULL) == 0);

    // Joins all threads
    assert(pthread_join(t1, NULL) == 0);
    assert(pthread_join(t2, NULL) == 0);
    assert(pthread_join(t3, NULL) == 0);

    assert_contents_ok(f3);
    assert(tfs_open(f1, 0) == -1);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");
    return 0;
}