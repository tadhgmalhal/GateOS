#include "tmpfs.h"
#include "../lib/string.h"
#include "../lib/kprintf.h"
#include "../mm/kheap.h"

#define TMPFS_MAX_CHILDREN 32
#define TMPFS_MAX_SIZE     (1024 * 1024)  // 1MB max per file

typedef struct tmpfs_node
{
    vfs_node_t          vfs;
    uint8_t            *data;
    uint32_t            data_size;
    struct tmpfs_node  *children[TMPFS_MAX_CHILDREN];
    char                child_names[TMPFS_MAX_CHILDREN][VFS_NAME_MAX];
    int                 child_count;
} tmpfs_node_t;

static tmpfs_node_t tmpfs_root_node;

static uint32_t tmpfs_read(vfs_node_t *node, uint32_t offset,
                            uint32_t size, uint8_t *buffer)
{
    tmpfs_node_t *tnode = (tmpfs_node_t *)node;

    if (!tnode->data || offset >= tnode->data_size)
    {
        return 0;
    }

    uint32_t available = tnode->data_size - offset;
    uint32_t to_read   = (size < available) ? size : available;

    memcpy(buffer, tnode->data + offset, to_read);
    return to_read;
}

static uint32_t tmpfs_write(vfs_node_t *node, uint32_t offset,
                             uint32_t size, const uint8_t *buffer)
{
    tmpfs_node_t *tnode = (tmpfs_node_t *)node;

    uint32_t end = offset + size;

    if (end > TMPFS_MAX_SIZE)
    {
        return 0;
    }

    if (end > tnode->data_size)
    {
        uint8_t *new_data = (uint8_t *)kmalloc(end);
        if (!new_data)
        {
            return 0;
        }

        if (tnode->data)
        {
            memcpy(new_data, tnode->data, tnode->data_size);
            kfree(tnode->data);
        }

        memset(new_data + tnode->data_size, 0, end - tnode->data_size);
        tnode->data      = new_data;
        tnode->data_size = end;
        node->size       = end;
    }

    memcpy(tnode->data + offset, buffer, size);
    return size;
}

static vfs_dirent_t *tmpfs_readdir(vfs_node_t *node, uint32_t index)
{
    tmpfs_node_t *tnode = (tmpfs_node_t *)node;

    if ((int)index >= tnode->child_count)
    {
        return 0;
    }

    static vfs_dirent_t dirent;
    strncpy(dirent.name, tnode->child_names[index], VFS_NAME_MAX - 1);
    dirent.name[VFS_NAME_MAX - 1] = '\0';
    dirent.inode = index;
    return &dirent;
}

static vfs_node_t *tmpfs_finddir(vfs_node_t *node, const char *name)
{
    tmpfs_node_t *tnode = (tmpfs_node_t *)node;

    for (int i = 0; i < tnode->child_count; i++)
    {
        if (strcmp(tnode->child_names[i], name) == 0)
        {
            return &tnode->children[i]->vfs;
        }
    }
    return 0;
}

static tmpfs_node_t *tmpfs_alloc_node(const char *name, uint32_t type)
{
    tmpfs_node_t *node = (tmpfs_node_t *)kmalloc(sizeof(tmpfs_node_t));
    if (!node)
    {
        return 0;
    }

    memset(node, 0, sizeof(tmpfs_node_t));
    strncpy(node->vfs.name, name, VFS_NAME_MAX - 1);
    node->vfs.name[VFS_NAME_MAX - 1] = '\0';
    node->vfs.type    = type;
    node->vfs.size    = 0;
    node->data        = 0;
    node->data_size   = 0;
    node->child_count = 0;

    if (type & VFS_FILE)
    {
        node->vfs.read  = tmpfs_read;
        node->vfs.write = tmpfs_write;
    }

    if (type & VFS_DIRECTORY)
    {
        node->vfs.readdir = tmpfs_readdir;
        node->vfs.finddir = tmpfs_finddir;
    }

    return node;
}

static int tmpfs_add_child(tmpfs_node_t *parent, tmpfs_node_t *child,
                            const char *name)
{
    if (parent->child_count >= TMPFS_MAX_CHILDREN)
    {
        return 0;
    }

    strncpy(parent->child_names[parent->child_count], name, VFS_NAME_MAX - 1);
    parent->child_names[parent->child_count][VFS_NAME_MAX - 1] = '\0';
    parent->children[parent->child_count] = child;
    parent->child_count++;
    return 1;
}

vfs_node_t *tmpfs_create_file(vfs_node_t *dir, const char *name)
{
    tmpfs_node_t *parent = (tmpfs_node_t *)dir;
    tmpfs_node_t *node   = tmpfs_alloc_node(name, VFS_FILE);
    if (!node)
    {
        return 0;
    }

    tmpfs_add_child(parent, node, name);
    return &node->vfs;
}

vfs_node_t *tmpfs_create_dir(vfs_node_t *dir, const char *name)
{
    tmpfs_node_t *parent = (tmpfs_node_t *)dir;
    tmpfs_node_t *node   = tmpfs_alloc_node(name, VFS_DIRECTORY);
    if (!node)
    {
        return 0;
    }

    tmpfs_add_child(parent, node, name);
    return &node->vfs;
}

vfs_node_t *tmpfs_get_root()
{
    return &tmpfs_root_node.vfs;
}

void tmpfs_init()
{
    memset(&tmpfs_root_node, 0, sizeof(tmpfs_node_t));
    strncpy(tmpfs_root_node.vfs.name, "tmp", VFS_NAME_MAX - 1);
    tmpfs_root_node.vfs.type    = VFS_DIRECTORY;
    tmpfs_root_node.vfs.readdir = tmpfs_readdir;
    tmpfs_root_node.vfs.finddir = tmpfs_finddir;
    tmpfs_root_node.child_count = 0;

    kprintf("tmpfs initialized.\n");
}