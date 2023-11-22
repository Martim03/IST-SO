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

    assert(tfs_write(fd1, content, sizeof(content)) == sizeof(content));

    assert(tfs_close(fd1) != -1);

    // Open the file
    fd1 = tfs_open(f1, 0);
    assert(fd1 != -1);

    // Unlinks it
    assert(tfs_unlink(f1) != -1);

    // Can still read the content
    char b1[sizeof(content)];
    assert(tfs_read(fd1, b1, sizeof(b1)) == sizeof(content));
    assert(memcmp(b1, content, sizeof(content)) == 0);

    // Can still close it
    assert(tfs_close(fd1) != -1);

    // Cant open anymore
    assert(tfs_open(f1, 0) == -1);

    // Create a file with the same path as the last one
    create(f2);

    // Read the file
    int fd2 = tfs_open(f2, 0);
    assert(fd2 != -1);

    char b2[sizeof(content)];
    assert(tfs_read(fd2, b2, sizeof(b2)) != -1);
    assert(memcmp(b2, content, sizeof(b2)) != 0); // The content is not the same as file 1

    assert(tfs_destroy() != -1);
    printf("Successful test.\n");
    return 0;
}