#ifndef DEVFS_H
#define DEVFS_H

#include "vfs.h"

void        devfs_init();
vfs_node_t *devfs_get_root();
void        devfs_register(const char *name, vfs_node_t *node);

#endif