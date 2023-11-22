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

    char Text[] = "SO PROJECT!!!";
    char f1[] = "/f1";
    char f2[] = "/f2";
    char text_path[] = "tests/customTEXT";

    // Create a file
    create(f1);

    // Same destination and source path
    assert(tfs_copy_from_external_fs(text_path, text_path) == -1);

    // Cant copy from a non-existent file
    assert(tfs_copy_from_external_fs(f2, f1) == -1);

    // Can copy to a non-existent file
    assert(tfs_copy_from_external_fs(text_path, f2) != -1);

    // Open the file
    int fd2 = tfs_open(f2, 0);
    assert(fd2 != -1);

    // Check the content
    char buffer2[15];
    ssize_t r2 = tfs_read(fd2, buffer2, sizeof(buffer2));
    assert(r2 == strlen(Text));

    assert(tfs_destroy() != -1);
    printf("Successful test.\n");
    return 0;
}
