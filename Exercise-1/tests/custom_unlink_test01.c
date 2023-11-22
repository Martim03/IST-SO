#include "../fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

char file_contents[] = "SO PROJECT!";
char f1[] = "/f1";
char l1[] = "/l1";
char l2[] = "/l2";


void assert_contents_ok(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    uint8_t buffer[sizeof(file_contents)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == sizeof(buffer));
    assert(memcmp(buffer, file_contents, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

void write_contents(char const *path) {
    int f = tfs_open(path, TFS_O_APPEND);
    assert(f != -1);

    assert(tfs_write(f, file_contents, sizeof(file_contents)) ==
           sizeof(file_contents));

    assert(tfs_close(f) != -1);
}

// Create file
void create(char fp[]) {
    int f = tfs_open(fp, TFS_O_CREAT);
    assert(f != -1);
    assert(tfs_close(f) != -1);
}

int main() {
    assert(tfs_init(NULL) != -1);

    // Create file
    create(f1);

    // Create soft_links
    assert(tfs_sym_link(f1, l1) != -1);
    assert(tfs_sym_link(f1, l2) != -1);

    // Unlink one of them
    assert(tfs_unlink(l1) != -1);

    // The link is no longer usable
    assert(tfs_open(l1, 0) == -1);

    // The other is still accessible
    assert(tfs_open(l2, 0) != -1);

    // Unlink the other
    assert(tfs_unlink(l2) != -1);

    // The link is no longer usable
    assert(tfs_open(l2, 0) == -1);

    // The original file is still accessible
    assert(tfs_open(f1, 0) != -1);

    assert(tfs_destroy() != -1);
    printf("Successful test.\n");
    return 0;
}