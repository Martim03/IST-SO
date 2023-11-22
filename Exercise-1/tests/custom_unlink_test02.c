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

    // Create a hard_link to the file
    assert(tfs_link(f1, l1) != -1);

    // Create a soft_link to the file
    assert(tfs_sym_link(f1, l2) != -1);

    //Unlink every file and link
    assert(tfs_unlink(l2) != -1);
    assert(tfs_unlink(l1) != -1);
    assert(tfs_unlink(f1) != -1);

    // Cant unlink them again
    assert(tfs_unlink(l2) == -1);
    assert(tfs_unlink(l1) == -1);
    assert(tfs_unlink(f1) == -1);

    assert(tfs_destroy() != -1);
    printf("Successful test.\n");
    return 0;
}