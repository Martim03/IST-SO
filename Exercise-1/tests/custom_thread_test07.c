#include "../fs/operations.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

uint8_t const file_contents[] = "SO PROJECT!";
char fA[] = "/f1";
char fB[] = "/f2";
char non_existent_file[] = "/nef";
char l1[] = "/l1";
char l2[] = "/l2";
char l3[] = "/l3";

void read_contents(char *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    char buffer[50];
    assert(tfs_read(f, buffer, sizeof(buffer)) != -1);

    assert(tfs_close(f) != -1);
}

void write_contents(char *path) {
    int f = tfs_open(path, TFS_O_APPEND);
    assert(f != -1);

    assert(tfs_write(f, file_contents, sizeof(file_contents)) ==
           sizeof(file_contents));

    assert(tfs_close(f) != -1);
}

void *th_run01() {
    write_contents(fA);
    assert(tfs_sym_link(fA, l1) != -1);
    assert(tfs_link(fB, l2) != -1);

    return 0;
}

void *th_run02() {
    write_contents(fB);
    assert(tfs_open(non_existent_file, 0) == -1);
    assert(tfs_link(fB, l3) != -1);
    read_contents(fA);
    read_contents(fB);

    return 0;
}

void create(char *f) {
    int fd = tfs_open(f, TFS_O_CREAT);
    assert(fd != -1);
    assert(tfs_close(fd) != -1);
}

int main() {
    assert(tfs_init(NULL) != -1);

    create(fA);
    create(fB);

    // Creates threads
    pthread_t t1, t2;

    // run1 - Creates a soft_link and tries to read through it
    assert(pthread_create(&t1, NULL, th_run01, NULL) == 0);
    // run2 - Creates a hard_link and writes through it
    assert(pthread_create(&t2, NULL, th_run02, NULL) == 0);

    // Joins all threads
    assert(pthread_join(t1, NULL) == 0);
    assert(pthread_join(t2, NULL) == 0);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");
    return 0;
}