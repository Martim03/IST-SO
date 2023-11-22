#include "../fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

char file_contents[] = "SO PROJECT!";
char f1[] = "/f1";
char f2[] = "/f2";
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

    // Create files
    create(f1);
    create(f2);

    // Create a soft
    assert(tfs_sym_link(f1, l1) != -1);

    // Write text through file path
    write_contents(f1);

    // Read text through soft_link
    assert_contents_ok(l1);

    // Unlink the original file
    assert(tfs_unlink(f1) != -1);

    // Link not usable
    assert(tfs_open(l1, 0) == -1);

    //-----------------------------------------

    // Create soft_link
    assert(tfs_sym_link(f2, l2) != -1);

    // Write text through soft_link
    write_contents(l2);

    // Read through the file path
    assert_contents_ok(f2);

    // Unlink the soft_link
    assert(tfs_unlink(l2) != -1);

    // File still accessible
    assert_contents_ok(f2);

    assert(tfs_destroy() != -1);
    printf("Successful test.\n");
    return 0;
}
