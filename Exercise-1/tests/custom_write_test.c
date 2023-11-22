#include "../fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {
    const char file_path[] = "/f1";

    assert(tfs_init(NULL) != -1);

    // Create file
    int fd = tfs_open(file_path, TFS_O_CREAT);
    assert(fd != -1);
    assert(tfs_close(fd) != -1);

    char b1[50];
    char b2[50];
    char b3[50];

    char a[] = "AAA";
    char b[] = "BBB";
    char c[] = "CCC";

    fd = tfs_open(file_path, TFS_O_APPEND);
    tfs_write(fd, a, strlen(a));
    tfs_close(fd);
    fd = tfs_open(file_path, 0);

    tfs_read(fd, b1, strlen(b1));
    tfs_close(fd);

    fd = tfs_open(file_path, TFS_O_APPEND);
    tfs_write(fd, b, strlen(b));
    tfs_close(fd);
    fd = tfs_open(file_path, 0);

    tfs_read(fd, b2, strlen(b2));
    tfs_close(fd);

    fd = tfs_open(file_path, TFS_O_APPEND);
    tfs_write(fd, c, strlen(c));
    tfs_close(fd);
    fd = tfs_open(file_path, 0);

    tfs_read(fd, b3, strlen(b3));
    tfs_close(fd);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");
}
