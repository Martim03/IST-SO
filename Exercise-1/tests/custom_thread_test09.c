#include "../fs/operations.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

/// ONLY TESTS DATA RACES - DOESNT PRODUCE OUTPUT (TEST WITH -FSANITIZE)

char file_contents[] = "a";
char f1[] = "/f1";
char f2[] = "/f2";

void read() {
    int f = tfs_open(f1, 0);
    assert(f != -1);

    char buffer[5000];

    assert(tfs_read(f, buffer, sizeof(buffer)) != -1);

    assert(tfs_close(f) != -1);
}

void write() {
    int f = tfs_open(f1, TFS_O_APPEND);
    assert(f != -1);

    assert(tfs_write(f, file_contents, strlen(file_contents)) ==
           strlen(file_contents));

    assert(tfs_close(f) != -1);
}

int open() {
    int f = tfs_open(f1, 0);
    assert(f != -1);
    return f;
}

void close(int f) {
    assert(tfs_close(f) != -1);
}

void create(char fp[]) {
    int fd = tfs_open(fp, TFS_O_CREAT);
    assert(fd != -1);
    assert(tfs_close(fd) != -1);
}

void *th_run01() {
    read();
    int a = open();
    read();
    write();
    int b = open();
    read();
    write();
    write();
    write();
    read();
    close(b);
    read();
    int c = open();
    close(a);
    read();
    write();
    int d = open();
    int e = open();
    int f = open();
    int g = open();
    read();
    read();
    read();
    write();
    close(d);
    close(e);
    read();
    close(f);
    write();
    close(c);
    write();
    read();
    close(g);
    return 0;
}

void *th_run02() {
    int a = open();
    close(a);
    int b = open();
    close(b);
    int c = open();
    close(c);
    int d = open();
    close(d);
    int e = open();
    close(e);
    read();
    write();
    read();
    write();
    read();
    write();
    read();
    write();
    int f = open();
    int g = open();
    int h = open();
    int i = open();
    int j = open();
    int k = open();
    read();
    read();
    write();
    read();
    read();
    read();
    write();
    read();
    read();
    close(f);
    close(g);
    close(h);
    close(i);
    close(j);
    close(k);
    return 0;
}

void *th_run03() {
    for (int i = 0; i < 1000; i++) {
        create(f2);
    }
    return 0;
}

void *th_run04() {
    for (int i = 0; i < 1000; i++) {
        tfs_unlink(f2);
    }
    return 0;
}

void *th_run05() {
    for (int i = 0; i < 1000; i++) {
        write();
    }
    return 0;
}

void *th_run06() {
    for (int i = 0; i < 1000; i++) {
        read();
    }
    return 0;
}

int main() {
    assert(tfs_init(NULL) != -1);

    // Creates threads
    pthread_t t1, t2, t3, t4, t5, t6;

    // Create a file
    create(f1);

    assert(pthread_create(&t1, NULL, th_run01, NULL) == 0);
    assert(pthread_create(&t2, NULL, th_run02, NULL) == 0);
    assert(pthread_create(&t3, NULL, th_run03, NULL) == 0);
    assert(pthread_create(&t4, NULL, th_run04, NULL) == 0);
    assert(pthread_create(&t5, NULL, th_run05, NULL) == 0);
    assert(pthread_create(&t6, NULL, th_run06, NULL) == 0);

    // Joins threads
    assert(pthread_join(t1, NULL) == 0);
    assert(pthread_join(t2, NULL) == 0);
    assert(pthread_join(t3, NULL) == 0);
    assert(pthread_join(t4, NULL) == 0);
    assert(pthread_join(t5, NULL) == 0);
    assert(pthread_join(t6, NULL) == 0);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");
    return 0;
}
