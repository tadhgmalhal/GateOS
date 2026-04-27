#ifndef USERSPACE_H
#define USERSPACE_H

#include <stdint.h>

void jump_to_userspace(uint32_t eip, uint32_t esp);

#endif