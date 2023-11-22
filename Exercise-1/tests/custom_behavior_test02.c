#include "../fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

char content[] = "SO PROJECT!";
// Same path
char f1[] = "/f1";
char f2[] = "/f1";

// Create file
void create(char fp[]) {
    int f = tfs_open(fp, TFS_O_CREAT);
    assert(f != -1);
    assert(tfs_close(f) != -1);
}

int main() {
    assert(tfs_init(NULL) != -1);

    // Create a file
    create(f1);

    // Write in the file
    int fd1 = tfs_open(f1, TFS_O_APPEND);
    assert(fd1 != -1);

    int fd2 = tfs_open(f1, 0);
    assert(fd2 != -1);

    assert(tfs_write(fd1, content, sizeof(content)) == sizeof(content));

    char b1[sizeof(content)];
    assert(tfs_read(fd2, b1, sizeof(b1)) == sizeof(b1));
    assert(memcmp(b1, content, sizeof(b1)) == 0);

    // Close the files
    assert(tfs_close(fd1) != -1);
    assert(tfs_close(fd2) != -1);

    assert(tfs_destroy() != -1);
    printf("Successful test.\n");
    return 0;
}