#include "../fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

uint8_t  file_contents[] = "AAAAAAAAAAAAA";
char  f1[] = "/f1";
char  f2[] = "/f2";
char  f3[] = "/f3";
char  f4[] = "/f4";
char  f5[] = "/f5";
char text_path[] = "tests/customTEXT";

void assert_contents_ok(char  *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    char buffer[sizeof(file_contents)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == sizeof(buffer));
    assert(memcmp(buffer, file_contents, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

void write_contents(char  *path) {
    int f = tfs_open(path, TFS_O_APPEND);
    assert(f != -1);

    assert(tfs_write(f, file_contents, sizeof(file_contents)) ==
           sizeof(file_contents));

    assert(tfs_close(f) != -1);
}

void *th_run01() {
    tfs_copy_from_external_fs(text_path, f1);
    return 0;
}

void *th_run02() {
    int f = tfs_open(f1, 0);
    assert(f != -1);

    char b1[50];
    tfs_read(f, b1, sizeof(b1));

    assert(tfs_close(f) != -1);

    write_contents(f2);
    write_contents(f3);
    write_contents(f4);
    write_contents(f5);
    assert_contents_ok(f2);
    assert_contents_ok(f3);
    assert_contents_ok(f4);
    assert_contents_ok(f5);

    f = tfs_open(f1, 0);
    assert(f != -1);

    char b2[50];
    tfs_read(f, b2, sizeof(b2));

    assert(strncmp(b2, "SO PROJECT!!!", 13) == 0);
    assert(tfs_close(f) != -1);

    assert(tfs_unlink(f1) != -1);
    return 0;
}

void create(char path[]) {
    int fd = tfs_open(path, TFS_O_CREAT);
    assert(fd != -1);
    assert(tfs_close(fd) != -1);
}

int main() {
    assert(tfs_init(NULL) != -1);

    create(f1);
    create(f2);
    create(f3);
    create(f4);
    create(f5);

    // Creates threads
    pthread_t t1, t2;

    // Run1 - Copies text from external file to f1
    assert(pthread_create(&t1, NULL, th_run01, NULL) == 0);
    // Run2 - Complete multiples instructions and then checks if f1 copied the correct text
    assert(pthread_create(&t2, NULL, th_run02, NULL) == 0);

    // Joins threads
    assert(pthread_join(t1, NULL) == 0);
    assert(pthread_join(t2, NULL) == 0);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");
    return 0;
}
