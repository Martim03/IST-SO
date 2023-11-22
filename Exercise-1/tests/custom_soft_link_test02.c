#include "../fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main() {
    assert(tfs_init(NULL) != -1);

    // Cant create a soft_link to a non-existent file
    assert(tfs_sym_link("/f1", "/l1") == -1);

    assert(tfs_destroy() != -1);
    printf("Successful test.\n");
    return 0;
}