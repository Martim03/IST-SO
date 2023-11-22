#include "operations.h"
#include "config.h"
#include "state.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "betterassert.h"



tfs_params tfs_default_params() {
    tfs_params params = {
        .max_inode_count = 64,
        .max_block_count = 1024,
        .max_open_files_count = 16,
        .block_size = 1024,
    };
    return params;
}

int tfs_init(tfs_params const *params_ptr) {
    tfs_params params;
    if (params_ptr != NULL) {
        params = *params_ptr;
    } else {
        params = tfs_default_params();
    }

    if (state_init(params) != 0) {
        return -1;
    }

    // create root inode
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
        return -1;
    }

    return 0;
}

int tfs_destroy() {
    if (state_destroy() != 0) {
        return -1;
    }
    return 0;
}

static bool valid_pathname(char const *name) {
    return name != NULL && strlen(name) > 1 && name[0] == '/';
}

/**
 * Looks for a file.
 *
 * Note: as a simplification, only a plain directory space (root directory only)
 * is supported.
 *
 * Input:
 *   - name: absolute path name
 *   - root_inode: the root directory inode
 * Returns the inumber of the file, -1 if unsuccessful.
 */
static int tfs_lookup(char const *name, inode_t const *root_inode) {
    if (!valid_pathname(name)) {
        return -1;
    }
    if (root_inode != inode_get(ROOT_DIR_INUM)) {
        return -1;
    }
    // skip the initial '/' character
    name++;
    return find_in_dir(root_inode, name);
}


/**
 * Open file.
 *
 * Input:
 *  - name: absolute path name
 *  - mode: open flags
 * Returns the file descriptor, -1 if unsuccessful.
 */
int tfs_open(char const *name, tfs_file_mode_t mode) {
    // Checks if the path name is valid
    if (!valid_pathname(name)) {
        return -1;
    }

    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL,
                  "tfs_open: root dir inode must exist");
    size_t offset;

    int inum = tfs_lookup(name, root_dir_inode);
    if (inum >= 0) {
        // The file already exists

        // Lock the inode
        wrlock(get_lock(inum));
        inode_t *inode = inode_get(inum);
        ALWAYS_ASSERT(inode != NULL, "tfs_open: directory files must have an inode");

        // If the file is a symlink, get the file it points to
        while (inode->i_node_type == T_SYMLINK) {
            // Get the file it points to
            char* filename = (char*)data_block_get(inode->i_data_block);

            // Get the inode number of the file points to
            int newinum = tfs_lookup(filename, root_dir_inode);
            if (newinum < 0) {
                rw_unlock(get_lock(inum));
                return -1;
            }

            // Lock the new inode and unlock the old one
            wrlock(get_lock(newinum));
            rw_unlock(get_lock(inum));

            inum = newinum;
            // Check if the file exists
            inode = inode_get(inum);
            if (inode == NULL) {
                rw_unlock(get_lock(inum));
                return -1;
            }
        }
        // Truncate (if requested)
        if (mode & TFS_O_TRUNC) {
            if (inode->i_size > 0) {
                data_block_free(inode->i_data_block);
                inode->i_size = 0;
            }
        }
        // Determine initial offset
        if (mode & TFS_O_APPEND) {
            offset = inode->i_size;
        } else {
            offset = 0;
        }
    } else if (mode & TFS_O_CREAT) {
        // The file does not exist; the mode specified that it should be created
        // Create inode
        inum = inode_create(T_FILE);
        if (inum < 0) {
            return -1; // no space in inode table
        }

        wrlock(get_lock(inum)); // Lock the inode

        // Add entry in the root directory
        if (add_dir_entry(root_dir_inode, name + 1, inum) == -1) {
            inode_delete(inum); // delete inode if failed to add entry
            rw_unlock(get_lock(inum)); // unlock the inode
            return -1; // no space in directory
        }

        offset = 0;
    } else {
        return -1;
    }

    // Add entry to the open file table and return the corresponding handle
    int added = add_to_open_file_table(inum, offset);

    rw_unlock(get_lock(inum)); // Unlock the inode
    return added;
    // Note: for simplification, if file was created with TFS_O_CREAT and there
    // is an error adding an entry to the open file table, the file is not
    // opened, but it remains created
}

/**
 * Creates a symlink to a file.
 * Adds an entry for the symlink in the root directory.
 *
 * Input:
 *   - target: absolute path name of the target file
 *   - link_name: absolute path name of the symlink
 * Returns 0 if successful, -1 otherwise.
 */
int tfs_sym_link(char const *target, char const *link_name) {
    // Checks if the path name is valid
    if (!valid_pathname(link_name) || !valid_pathname(target)) {
        return -1;
    }

    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL,
                  "tfs_sym_link: root dir inode must exist");

    // Check if the target file exists
    int target_inumber = tfs_lookup(target, root_dir_inode);
    if (target_inumber < 0) {
        return -1;
    }

    rdlock(get_link_lock(target_inumber));

    // Check if the inode exists
    inode_t *target_inode = inode_get(target_inumber);
    if (target_inode == NULL) {
        rw_unlock(get_link_lock(target_inumber));
        return -1;
    }


    // Check if an equally named file already exists
    if (tfs_lookup(link_name, root_dir_inode) != -1) {
        rw_unlock(get_link_lock(target_inumber));
        return -1;
    }

    // Create inode for the symlink
    int inumber = inode_create(T_SYMLINK);
    if (inumber == -1) {
        rw_unlock(get_link_lock(target_inumber));
        return -1; // no space in inode table
    }

    wrlock(get_link_lock(inumber));

    // Check if inode was created successfully
    inode_t *inode = inode_get(inumber);
    if (inode == NULL) {
        rw_unlock(get_link_lock(target_inumber));
        rw_unlock(get_link_lock(inumber));
        return -1;
    }

    // Copy the target file name to the data block of the symlink
    if (strcpy((char*)data_block_get(inode->i_data_block), target) == NULL) {
        rw_unlock(get_link_lock(inumber));
        rw_unlock(get_link_lock(target_inumber));
        return -1;
    }
    // Set the size of the symlink
    inode->i_size = sizeof(target);

    // Add entry in the root directory
    int dir_entry = add_dir_entry(root_dir_inode, link_name + 1, inumber);

    rw_unlock(get_link_lock(inumber));
    rw_unlock(get_link_lock(target_inumber));
    return dir_entry;
}


/**
 * Creates a hardlink to a file.
 * Adds an entry for the hardlink in the root directory.
 *
 * Input:
 *   - target: absolute path name of the target file
 *   - link_name: absolute path name of the symlink
 * Returns 0 if successful, -1 otherwise.
 */
int tfs_link(char const *target, char const *link_name) {
    // Checks if the path name is valid
    if (!valid_pathname(link_name) || !valid_pathname(target)) {
        return -1;
    }

    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL,
                  "tfs_sym_link: root dir inode must exist");

    // Get the inode number of the target file
    int inumber = tfs_lookup(target, root_dir_inode);
    wrlock(get_link_lock(inumber));

    // Check if the target file exists
    if (inumber < 0) {
        rw_unlock(get_link_lock(inumber));
        return -1;
    }

    // Check if the inode exists
    inode_t *inode = inode_get(inumber);
    if (inode == NULL){
        rw_unlock(get_link_lock(inumber));
        return -1;
    }

    // if it is a symlink or directory, unable to hardlink
    if (inode->i_node_type != T_FILE) {
        rw_unlock(get_link_lock(inumber));
        return -1;
    }

    // if the link name already exists, unable to hardlink
    if (tfs_lookup(link_name, root_dir_inode) != -1) {
        rw_unlock(get_link_lock(inumber));
        return -1;
    }

    // Add entry in the root directory
    int dir_entry = add_dir_entry(root_dir_inode, link_name + 1, inumber);
    if (dir_entry < 0) {
        rw_unlock(get_link_lock(inumber));
        return -1;
    }

    // increment the hardlink count
    inode->hard_links++;

    rw_unlock(get_link_lock(inumber));
    return dir_entry;
}

/**
 * Close file.
 *
 * Input:
 *  - fhandle: file handle of the file to close
 * Returns 0 if successful, -1 otherwise.
 */
int tfs_close(int fhandle) {
    // Lock the open file entry
    wrlock(get_entry_lock(fhandle));
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        rw_unlock(get_entry_lock(fhandle));
        return -1; // invalid fd
    }

    // Remove the entry from the open file table
    remove_from_open_file_table(fhandle);

    // Unlock the open file entry
    rw_unlock(get_entry_lock(fhandle));
    return 0;
}


/**
 * Write to file.
 *
 * Input:
 * - fhandle: file handle of the file to write to
 * - buffer: buffer containing the data to write
 * - to_write: number of bytes to write
 * Returns the number of bytes written if successful, -1 otherwise.
 */
ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {
    // Lock the open file entry
    wrlock(get_entry_lock(fhandle));
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        rw_unlock(get_entry_lock(fhandle));
        return -1;
    }

    // Lock the inode of the file to write to
    wrlock(get_lock(file->of_inumber));

    // Get the inode of the file to write to
    inode_t *inode = inode_get(file->of_inumber);
    ALWAYS_ASSERT(inode != NULL, "tfs_write: inode of open file deleted");

    // Determine how many bytes to write
    size_t block_size = state_block_size();
    if (to_write + file->of_offset > block_size) {
        to_write = block_size - file->of_offset;
    }

    if (to_write > 0) {
        if (inode->i_size == 0) {
            // If empty file, allocate new block
            int bnum = data_block_alloc();
            if (bnum == -1) {
                rw_unlock(get_lock(file->of_inumber));
                rw_unlock(get_entry_lock(fhandle));
                return -1; // no space
            }

            inode->i_data_block = bnum;
        }

        void *block = data_block_get(inode->i_data_block);
        ALWAYS_ASSERT(block != NULL, "tfs_write: data block deleted mid-write");

        // Perform the actual write
        memcpy(block + file->of_offset, buffer, to_write);

        // The offset associated with the file handle is incremented accordingly
        file->of_offset += to_write;
        if (file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }
    }
    // Unlock the inode and the open file entry
    rw_unlock(get_lock(file->of_inumber));
    rw_unlock(get_entry_lock(fhandle));
    return (ssize_t)to_write;
}


/**
 * Read from file.
 *
 * Input:
 * - fhandle: file handle of the file to read from
 * - buffer: buffer to store the data read
 * - len: number of bytes to read
 * Returns the number of bytes read if successful, -1 otherwise.
 */
ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    // Lock the open file entry
    rdlock(get_entry_lock(fhandle));
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        rw_unlock(get_entry_lock(fhandle));
        return -1;
    }

    // Get the inode number from the open file table entry
    int inumber = file->of_inumber;
    // Lock the inode of the file to read from
    rdlock(get_lock(inumber));

    // From the open file table entry, we get the inode
    inode_t const *inode = inode_get(inumber);
    ALWAYS_ASSERT(inode != NULL, "tfs_read: inode of open file deleted");

    // Determine how many bytes to read
    size_t to_read = inode->i_size - file->of_offset;
    if (to_read > len) {
        to_read = len;
    }
    if (to_read > 0) {
        void *block = data_block_get(inode->i_data_block);
        ALWAYS_ASSERT(block != NULL, "tfs_read: data block deleted mid-read");

        // Perform the actual read
        memcpy(buffer, block + file->of_offset, to_read);
        // The offset associated with the file handle is incremented accordingly
        file->of_offset += to_read;
    }
    // Unlock the inode and the open file entry
    rw_unlock(get_lock(inumber));
    rw_unlock(get_entry_lock(fhandle));
    return (ssize_t)to_read;
}


/**
 * Delete a hardlink or symlink.
 * Removes the link from the root directory.
 *
 * Input:
 *   - target: absolute path name of the target link
 * Returns 0 if successful, -1 otherwise.
 */
int tfs_unlink(char const *target) {
    // Checks if the path name is valid
    if (!valid_pathname(target)) {
        return -1;
    }

    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL,
                  "tfs_sym_link: root dir inode must exist");

    // Check if the target file exists
    int inumber = tfs_lookup(target, root_dir_inode);
    if (inumber < 0) {
        return -1;
    }

    // Lock the inode of the target file
    wrlock(get_lock(inumber));
    inode_t *inode = inode_get(inumber);
    if (inode == NULL) {
        rw_unlock(get_lock(inumber));
        return -1;
    }

    // Remove entry from the root directory
    int cleared = clear_dir_entry(root_dir_inode, target + 1);
    if (cleared < 0) {
        rw_unlock(get_lock(inumber));
        return -1;
    }

    // If the target inode only has 1 hard link or
    // is a symlink (has 1 hard link)
    if (inode->hard_links == 1) {
        inode_delete(inumber);
    }

    // If there is more than 1 hard link, simply unlink
    else if (inode->hard_links > 1) {
        inode->hard_links--;
    }

    rw_unlock(get_lock(inumber));
    return cleared;
}


/**
 * Copy a file from an external FileSystem into TFS.
 * Files have a maximum size of 1 block, if the source file is larger
 * than 1 block, it will be truncated.
 *
 * Input:
 * - source_path: absolute path name of the source file
 * - dest_path: absolute path name of the destination file
 * Returns 0 if successful, -1 otherwise.
 */
int tfs_copy_from_external_fs(char const *source_path, char const *dest_path) {
    // Checks if the path name is valid
    if (!valid_pathname(dest_path)) {
        return -1;
    }

    // Open the source file
    FILE *fd = fopen(source_path, "r");
    if (fd == NULL) {
        return -1;
    }

    // Create the destination file
    int new = tfs_open(dest_path, TFS_O_CREAT | TFS_O_TRUNC );
    if (new < 0) {
        fclose(fd);
        return -1;
    }

    char buffer[128];
    memset(buffer,0,sizeof(buffer));
    long unsigned int bytes_read;
    long int bytes_written;
    long int total_bytes = 0;

    // Read from the source file and write to the destination file
    while ((bytes_read = fread(buffer, sizeof(char), sizeof(buffer),fd)) > 0) {
        bytes_written = tfs_write(new ,buffer, bytes_read);
        memset(buffer, 0, sizeof(buffer));
        total_bytes += bytes_written;
        if (total_bytes >= get_block_size()) {
            tfs_close(new);
            fclose(fd);
            return 0;
        }
        if (bytes_written != bytes_read) {
            // If it is not able to write all the bytes, return an error
            fclose(fd);
            tfs_close(new);
            return -1;
        }
    }

    fclose(fd);
    tfs_close(new);

    return 0;
}