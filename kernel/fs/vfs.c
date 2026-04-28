#include "vfs.h"
#include "../lib/string.h"
#include "../lib/kprintf.h"
#include "../mm/kheap.h"

static vfs_mount_t mounts[VFS_MAX_MOUNTS];
static int         mount_count = 0;
static vfs_node_t *vfs_root   = 0;

void vfs_init()
{
    mount_count = 0;
    vfs_root    = 0;

    for (int i = 0; i < VFS_MAX_MOUNTS; i++)
    {
        mounts[i].path[0] = '\0';
        mounts[i].root    = 0;
    }

    kprintf("VFS initialized.\n");
}

void vfs_mount(const char *path, vfs_node_t *root)
{
    if (mount_count >= VFS_MAX_MOUNTS)
    {
        kprintf("VFS: mount table full\n");
        return;
    }

    strncpy(mounts[mount_count].path, path, VFS_NAME_MAX - 1);
    mounts[mount_count].path[VFS_NAME_MAX - 1] = '\0';
    mounts[mount_count].root = root;
    mount_count++;

    // if mounting at root set vfs_root
    if (strcmp(path, "/") == 0)
    {
        vfs_root = root;
    }

    kprintf("VFS: mounted %s\n", path);
}

static vfs_node_t *vfs_resolve_mount(const char *path)
{
    // find the longest matching mount point
    int         best_len   = -1;
    vfs_node_t *best_mount = 0;

    for (int i = 0; i < mount_count; i++)
    {
        int len = strlen(mounts[i].path);
        if (strncmp(path, mounts[i].path, len) == 0)
        {
            if (len > best_len)
            {
                best_len   = len;
                best_mount = mounts[i].root;
            }
        }
    }

    return best_mount;
}

vfs_node_t *vfs_open(const char *path)
{
    if (!path || path[0] != '/')
    {
        return 0;
    }

    // find best matching mount point
    int         best_len   = -1;
    vfs_node_t *best_mount = 0;
    const char *best_path  = 0;

    for (int i = 0; i < mount_count; i++)
    {
        int len = strlen(mounts[i].path);
        if (strncmp(path, mounts[i].path, len) == 0)
        {
            if (len > best_len)
            {
                best_len   = len;
                best_mount = mounts[i].root;
                best_path  = mounts[i].path;
            }
        }
    }

    if (!best_mount)
    {
        return 0;
    }

    // skip past the mount point in the path
    const char *relative = path + best_len;

    // skip any leading slashes
    while (*relative == '/')
    {
        relative++;
    }

    // if nothing left after mount point return the mount root
    if (*relative == '\0')
    {
        if (best_mount->open)
        {
            best_mount->open(best_mount);
        }
        return best_mount;
    }

    // walk remaining path components using finddir
    vfs_node_t *current = best_mount;

    char buf[VFS_NAME_MAX];
    strncpy(buf, relative, VFS_NAME_MAX - 1);
    buf[VFS_NAME_MAX - 1] = '\0';

    char *token = buf;

    while (*token != '\0')
    {
        char *next = token;
        while (*next != '/' && *next != '\0')
        {
            next++;
        }

        int has_more = (*next == '/');
        *next = '\0';

        if (!current->finddir)
        {
            return 0;
        }

        current = current->finddir(current, token);
        if (!current)
        {
            return 0;
        }

        if (has_more)
        {
            token = next + 1;
            while (*token == '/')
            {
                token++;
            }
        }
        else
        {
            break;
        }
    }

    if (current && current->open)
    {
        current->open(current);
    }

    return current;
}

uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    if (!node || !node->read)
    {
        return 0;
    }
    return node->read(node, offset, size, buffer);
}

uint32_t vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, const uint8_t *buffer)
{
    if (!node || !node->write)
    {
        return 0;
    }
    return node->write(node, offset, size, buffer);
}

void vfs_close(vfs_node_t *node)
{
    if (!node || !node->close)
    {
        return;
    }
    node->close(node);
}

vfs_dirent_t *vfs_readdir(vfs_node_t *node, uint32_t index)
{
    if (!node || !node->readdir || !(node->type & VFS_DIRECTORY))
    {
        return 0;
    }
    return node->readdir(node, index);
}

vfs_node_t *vfs_finddir(vfs_node_t *node, const char *name)
{
    if (!node || !node->finddir || !(node->type & VFS_DIRECTORY))
    {
        return 0;
    }
    return node->finddir(node, name);
}