#include "../fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

char file_contents[] = "SO PROJECT!!!";
char text_path[] = "tests/customTEXT";
char  f1[] = "/f1";
char  f2[] = "/f2";
char  f3[] = "/f3";
char  f4[] = "/f4";
char  f5[] = "/f5";
char  l1[] = "/l1";
char  l2[] = "/l2";
char  l3[] = "/l3";
char  l4[] = "/l4";
char  l5[] = "/l5";

void assert_empty_file(char  fp[]) {
    int f = tfs_open(fp, 0);
    assert(f != -1);

    char buffer[sizeof(file_contents)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

void assert_contents_ok(char fp[]) {
    int f = tfs_open(fp, 0);
    assert(f != -1);

    char buffer[sizeof(file_contents)];
    tfs_read(f, buffer, sizeof(buffer));
    /*printf("buffer :%s .\n", buffer);
    printf("fileCn :%s .\n", file_contents);
    printf("size %d\n", strncmp(buffer, file_contents, strlen(file_contents)));*/
    assert(strncmp(buffer, file_contents, strlen(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

void assert_contents(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    uint8_t buffer[sizeof(file_contents)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == sizeof(buffer));
    assert(memcmp(buffer, file_contents, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

void write_contents(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    assert(tfs_write(f, file_contents, sizeof(file_contents)) ==
           sizeof(file_contents));

    assert(tfs_close(f) != -1);
}

int open(char fp[]) {
    int f = tfs_open(fp, 0);
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
    assert_empty_file(f1);
    create(f4);
    assert_empty_file(f4);
    assert(tfs_sym_link(f1, l1) != -1);
    assert(tfs_copy_from_external_fs(text_path, l1) != -1);
    assert(tfs_link(f4, l2) != -1);
    write_contents(l2);
    assert(tfs_unlink(l1) != -1);
    assert(tfs_unlink(l2) != -1);
    assert(tfs_link(f1, l2) != -1);
    assert(tfs_unlink(f1) != -1);
    assert(tfs_unlink(f1) == -1);
    return 0;
}

void *th_run02() {
    create(f3);
    assert_empty_file(f3);
    assert_empty_file(f2);
    assert(tfs_link(f2, l3) != -1);
    assert(tfs_link(l3, l4) != -1);
    tfs_copy_from_external_fs(text_path, l4);
    assert(tfs_sym_link(f2, l5) != -1);
    assert(tfs_unlink(l3) != -1);
    assert(tfs_link(f3 ,l3) != -1);
    assert(tfs_unlink(f3) != -1);
    return 0;
}

int main() {
    assert(tfs_init(NULL) != -1);

    create(f1);
    create(f2);

    // Creates threads
    pthread_t t1, t2;

    // Every thread runs multiple instructions in a single file
    assert(pthread_create(&t1, NULL, th_run01, NULL) == 0);
    assert(pthread_create(&t2, NULL, th_run02, NULL) == 0);

    // Joins threads
    assert(pthread_join(t1, NULL) == 0);
    assert(pthread_join(t2, NULL) == 0);

    // Checks if the output was correct
    assert_contents_ok(l2);
    assert_contents_ok(l5);
    assert_empty_file(l3);
    assert_contents_ok(f4);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");
    return 0;
}
