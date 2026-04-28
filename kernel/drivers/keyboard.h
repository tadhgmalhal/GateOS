#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../fs/vfs.h"

void        keyboard_init();
vfs_node_t *keyboard_get_device();

#endif