#ifndef TMPFS_H
#define TMPFS_H

#include "vfs.h"

void        tmpfs_init();
vfs_node_t *tmpfs_get_root();
vfs_node_t *tmpfs_create_file(vfs_node_t *dir, const char *name);
vfs_node_t *tmpfs_create_dir(vfs_node_t *dir, const char *name);

#endif