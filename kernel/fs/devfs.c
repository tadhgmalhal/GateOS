#include "devfs.h"
#include "../lib/string.h"
#include "../lib/kprintf.h"
#include "../mm/kheap.h"

#define DEVFS_MAX_ENTRIES 32

static vfs_node_t  devfs_root;
static vfs_node_t *devfs_entries[DEVFS_MAX_ENTRIES];
static char        devfs_names[DEVFS_MAX_ENTRIES][VFS_NAME_MAX];
static int         devfs_count = 0;

// null device — discards writes, returns EOF on read
static uint32_t null_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    (void)node; (void)offset; (void)size; (void)buffer;
    return 0;
}

static uint32_t null_write(vfs_node_t *node, uint32_t offset, uint32_t size, const uint8_t *buffer)
{
    (void)node; (void)offset; (void)buffer;
    return size;
}

// zero device — returns infinite zeros on read
static uint32_t zero_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    (void)node; (void)offset;
    memset(buffer, 0, size);
    return size;
}

static uint32_t zero_write(vfs_node_t *node, uint32_t offset, uint32_t size, const uint8_t *buffer)
{
    (void)node; (void)offset; (void)buffer;
    return size;
}

// devfs root readdir
static vfs_dirent_t *devfs_readdir(vfs_node_t *node, uint32_t index)
{
    (void)node;

    if ((int)index >= devfs_count)
    {
        return 0;
    }

    static vfs_dirent_t dirent;
    strncpy(dirent.name, devfs_names[index], VFS_NAME_MAX - 1);
    dirent.name[VFS_NAME_MAX - 1] = '\0';
    dirent.inode = index;
    return &dirent;
}

// devfs root finddir
static vfs_node_t *devfs_finddir(vfs_node_t *node, const char *name)
{
    (void)node;

    for (int i = 0; i < devfs_count; i++)
    {
        if (strcmp(devfs_names[i], name) == 0)
        {
            return devfs_entries[i];
        }
    }
    return 0;
}

void devfs_register(const char *name, vfs_node_t *node)
{
    if (devfs_count >= DEVFS_MAX_ENTRIES)
    {
        kprintf("devfs: too many entries\n");
        return;
    }

    strncpy(devfs_names[devfs_count], name, VFS_NAME_MAX - 1);
    devfs_names[devfs_count][VFS_NAME_MAX - 1] = '\0';
    devfs_entries[devfs_count] = node;
    devfs_count++;

    kprintf("devfs: registered /dev/%s\n", name);
}

vfs_node_t *devfs_get_root()
{
    return &devfs_root;
}

void devfs_init()
{
    // set up root node
    memset(&devfs_root, 0, sizeof(vfs_node_t));
    strncpy(devfs_root.name, "dev", VFS_NAME_MAX - 1);
    devfs_root.type    = VFS_DIRECTORY;
    devfs_root.readdir = devfs_readdir;
    devfs_root.finddir = devfs_finddir;

    devfs_count = 0;

    // create and register /dev/null
    vfs_node_t *null_node = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
    memset(null_node, 0, sizeof(vfs_node_t));
    strncpy(null_node->name, "null", VFS_NAME_MAX - 1);
    null_node->type  = VFS_CHARDEVICE;
    null_node->read  = null_read;
    null_node->write = null_write;
    devfs_register("null", null_node);

    // create and register /dev/zero
    vfs_node_t *zero_node = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
    memset(zero_node, 0, sizeof(vfs_node_t));
    strncpy(zero_node->name, "zero", VFS_NAME_MAX - 1);
    zero_node->type  = VFS_CHARDEVICE;
    zero_node->read  = zero_read;
    zero_node->write = zero_write;
    devfs_register("zero", zero_node);

    kprintf("devfs initialized.\n");
}