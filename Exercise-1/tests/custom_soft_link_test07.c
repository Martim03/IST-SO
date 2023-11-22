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

int main() {
    assert(tfs_init(NULL) != -1);
    uint8_t buffer[sizeof(file_contents)];
    // Create file
    int f = tfs_open(f1, TFS_O_CREAT);
    assert(f != -1);


    // Write to the file
    assert(tfs_write(f, file_contents, strlen(file_contents)) == strlen(file_contents));
    assert(tfs_close(f) != -1);

    // Read from the file
    f = tfs_open(f1, 0);
    tfs_read(f, buffer, strlen(file_contents));
    assert(tfs_close(f) != -1);

    // Create a soft_link to the file
    assert(tfs_sym_link(f1, l1) != -1);

    // Create a soft_link to the soft_link
    assert(tfs_sym_link(l1, l2) != -1);


    // Read through the soft_link l2
    f = tfs_open(l2, 0);
    assert(f != -1);
    assert(tfs_read(f, buffer, sizeof(buffer)) == strlen(file_contents));
    assert(memcmp(buffer, file_contents, strlen(file_contents)) == 0);
    assert(tfs_close(f) != -1);

    // Unlinks the soft_link l1
    assert(tfs_unlink(l1) != -1);

    // Try to open the soft_link l2
    f = tfs_open(l2, TFS_O_APPEND);
    assert(f == -1);
    
    assert(tfs_destroy() != -1);
    printf("Successful test.\n");
    return 0;
}