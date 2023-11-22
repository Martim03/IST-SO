#include "../fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void create(char fp[]) {
    int f = tfs_open(fp, TFS_O_CREAT);
    assert(f != -1);
    assert(tfs_close(f) != -1);
}

int main() {
    assert(tfs_init(NULL) != -1);

    char f1[] = "/f1";
    char large_path[] = "tests/customLargeTEXT";

    // Create a file
    create(f1);

    // Copy from a file that has more than 1024 bytes (1030 in this case)
    assert(tfs_copy_from_external_fs(large_path, f1) != -1);

    // Open the file
    int fd1 = tfs_open(f1, 0);
    assert(fd1 != -1);

    // The file only copies 1024 out of the 1030 bytes
    char buffer[1030];
    ssize_t r1 = tfs_read(fd1, buffer, sizeof(buffer));
    assert(r1 == 1024);

    assert(tfs_destroy() != -1);
    printf("Successful test.\n");
    return 0;
}
