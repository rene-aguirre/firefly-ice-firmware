#ifndef FD_SYSTEM_H
#define FD_SYSTEM_H

#include "fd_binary.h"

#define VENDOR 0x2333
#define PRODUCT 0x0002
#define HARDWARE_MAJOR 1
#define HARDWARE_MINOR 3

#define HARDWARE_ID_SIZE 16

// 16 byte hardware id: vendor, product, version (major, minor), unique id
void fd_get_hardware_id(fd_binary_t *binary);

#endif