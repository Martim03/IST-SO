#include "../fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

char file_contents[] = "SO PROJECT!";
char f1[] = "/f1";
char l1[] = "/l1";

void write_contents(char const *path) {
    int f = tfs_open(path, TFS_O_APPEND);
    assert(f != -1);

    assert(tfs_write(f, file_contents, sizeof(file_contents)) ==
           sizeof(file_contents));

    assert(tfs_close(f) != -1);
}

void check_content(int f) {
    char buffer[sizeof(file_contents)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == sizeof(buffer));
    assert(memcmp(buffer, file_contents, sizeof(buffer)) == 0);
}

int main() {
    assert(tfs_init(NULL) != -1);

    // Create a file
    int f = tfs_open(f1, TFS_O_CREAT);
    assert(f != -1);
    assert(tfs_close(f) != -1);

    // Create a soft_link to that file
    assert(tfs_sym_link(f1, l1) != -1);

    // Write through the link
    write_contents(l1);

    // Open the link
    int l = tfs_open(l1, 0);
    assert(l != -1);

    // Unlink it
    assert(tfs_unlink(l1) != -1);

    // Can still use it
    check_content(l);

    // Can still close it
    assert(tfs_close(l) != -1);

    // Cant open anymore
    assert(tfs_open(l1, 0) == -1);

    assert(tfs_destroy() != -1);
    printf("Successful test.\n");
    return 0;
}