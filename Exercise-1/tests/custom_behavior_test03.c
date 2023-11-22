#include "../fs/operations.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

char file_contents[] = "SO PROJECT!";
char const fp[] = "/f1";

void assert_contents_ok(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);
    char content[] = "SO PROJECT!SO PROJECT!SO PROJECT!";

    char buffer[strlen(content)];
    tfs_read(f, buffer, strlen(content));
    assert(memcmp(buffer, content, strlen(content)) == 0);

    assert(tfs_close(f) != -1);
}

void write_contents(char const *path) {
    int f = tfs_open(path, TFS_O_APPEND);
    assert(f != -1);

    assert(tfs_write(f, file_contents, strlen(file_contents)) ==
           strlen(file_contents));

    assert(tfs_close(f) != -1);
}

int main() {
    assert(tfs_init(NULL) != -1);
    int fd = tfs_open(fp, TFS_O_CREAT);
    assert(fd != -1);
    assert(tfs_close(fd) != -1);

    write_contents(fp);
    write_contents(fp);
    write_contents(fp);

    assert_contents_ok(fp);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");
    return 0;
}
