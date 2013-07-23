#ifndef FD_UPDATE_H
#define FD_UPDATE_H

#include <stdint.h>

#define FD_UPDATE_METADATA_FLAG_ENCRYPTED 0x00000001

typedef struct {
    uint32_t flags;
    uint32_t length;
    uint8_t hash[20];
    uint8_t crypt_hash[20];
    uint8_t crypt_iv[16];
} fd_update_metadata_t;

#define FD_UPDATE_COMMIT_FAIL_HASH_MISMATCH 1
#define FD_UPDATE_COMMIT_FAIL_CRYPT_HASH_MISMATCH 2

void fd_update_get_sector_hash(uint32_t sector, uint8_t *hash);
void fd_update_erase_sector(uint32_t sector);
void fd_update_write_page(uint32_t page, uint8_t *data);
uint8_t fd_update_commit(fd_update_metadata_t *metadata);

#endif