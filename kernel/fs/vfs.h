#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>

#define VFS_NAME_MAX 128

// node type flags
#define VFS_FILE        0x01
#define VFS_DIRECTORY   0x02
#define VFS_CHARDEVICE  0x04
#define VFS_BLOCKDEVICE 0x08
#define VFS_PIPE        0x10
#define VFS_SYMLINK     0x20
#define VFS_MOUNTPOINT  0x40

struct vfs_node;

// directory entry — returned by readdir
typedef struct
{
    char     name[VFS_NAME_MAX];
    uint32_t inode;
} vfs_dirent_t;

// function pointer types
typedef uint32_t (*vfs_read_t)    (struct vfs_node *node, uint32_t offset, uint32_t size, uint8_t *buffer);
typedef uint32_t (*vfs_write_t)   (struct vfs_node *node, uint32_t offset, uint32_t size, const uint8_t *buffer);
typedef void     (*vfs_open_t)    (struct vfs_node *node);
typedef void     (*vfs_close_t)   (struct vfs_node *node);
typedef vfs_dirent_t *(*vfs_readdir_t) (struct vfs_node *node, uint32_t index);
typedef struct vfs_node *(*vfs_finddir_t)(struct vfs_node *node, const char *name);

typedef struct vfs_node
{
    char     name[VFS_NAME_MAX];
    uint32_t inode;
    uint32_t size;
    uint32_t type;
    uint32_t permissions;
    uint32_t uid;
    uint32_t gid;

    vfs_read_t    read;
    vfs_write_t   write;
    vfs_open_t    open;
    vfs_close_t   close;
    vfs_readdir_t readdir;
    vfs_finddir_t finddir;

    struct vfs_node *ptr;  // used for mountpoints and symlinks
} vfs_node_t;

// mount table entry
typedef struct
{
    char        path[VFS_NAME_MAX];
    vfs_node_t *root;
} vfs_mount_t;

#define VFS_MAX_MOUNTS 16

void        vfs_init();
void        vfs_mount(const char *path, vfs_node_t *root);
vfs_node_t *vfs_open(const char *path);
uint32_t    vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
uint32_t    vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, const uint8_t *buffer);
void        vfs_close(vfs_node_t *node);
vfs_dirent_t *vfs_readdir(vfs_node_t *node, uint32_t index);
vfs_node_t   *vfs_finddir(vfs_node_t *node, const char *name);

#endif