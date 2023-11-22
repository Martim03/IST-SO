#include "../fs/operations.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

/// ONLY TESTS DATA RACES - DOESNT PRODUCE OUTPUT (TEST WITH -FSANITIZE)

char file_contents[] = "SO PROJECT!";
char f1[] = "/f1";
char l1[] = "/l1";
char l2[] = "/l2";
char l3[] = "/l3";
char l4[] = "/l4";

void *th_run01() {
    tfs_link(f1, l1);
    tfs_sym_link(l1, l2);
    tfs_sym_link(f1, l3);
    tfs_unlink(l2);
    tfs_unlink(l3);
    tfs_link(f1, l3);
    tfs_unlink(l1);
    tfs_link(l3, l1);
    tfs_unlink(l1);
    tfs_unlink(l1);
    tfs_sym_link(f1, l2);
    tfs_unlink(l3);
    tfs_sym_link(l2, l3);
    tfs_unlink(l2);
    tfs_link(f1, f1);
    tfs_unlink(l3);
    tfs_link(l1, l2);
    tfs_sym_link(f1, l3);
    tfs_unlink(l1);
    tfs_unlink(l2);
    tfs_unlink(l3);
    return 0;
}

void *th_run02() {
    tfs_unlink(l1);
    tfs_sym_link(f1, l3);
    tfs_link(f1, l2);
    tfs_unlink(l2);
    tfs_unlink(l3);
    tfs_link(f1, l1);
    tfs_sym_link(l1, l2);
    tfs_sym_link(l1, l3);
    tfs_unlink(l1);
    tfs_link(f1, l3);
    tfs_unlink(l2);
    tfs_unlink(l3);
    tfs_link(f1, l1);
    tfs_link(l1, l2);
    tfs_sym_link(l2, l3);
    tfs_unlink(l3);
    tfs_unlink(l1);
    tfs_sym_link(f1, l1);
    tfs_unlink(l1);
    tfs_unlink(l2);
    return 0;
}

void *th_run03() {
    for (int i = 0; i < 1000; i++) {
        tfs_link(f1, l4);
    }
    return 0;
}

void *th_run04() {
    for (int i = 0; i < 1000; i++) {
        tfs_unlink(l4);
    }
    return 0;
}

void create(char fp[]) {
    int fd = tfs_open(fp, TFS_O_CREAT);
    assert(fd != -1);
    assert(tfs_close(fd) != -1);
}

int main() {
    assert(tfs_init(NULL) != -1);

    // Creates threads
    pthread_t t1, t2, t3, t4;

    assert(pthread_create(&t1, NULL, th_run01, NULL) == 0);
    assert(pthread_create(&t2, NULL, th_run02, NULL) == 0);
    assert(pthread_create(&t3, NULL, th_run03, NULL) == 0);
    assert(pthread_create(&t4, NULL, th_run04, NULL) == 0);

    // Joins threads
    assert(pthread_join(t1, NULL) == 0);
    assert(pthread_join(t2, NULL) == 0);
    assert(pthread_join(t3, NULL) == 0);
    assert(pthread_join(t4, NULL) == 0);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");
    return 0;
}
