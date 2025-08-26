
#ifndef LFS_PORT_H
#define LFS_PORT_H

#include "lfs.h"


#define LFS_FLASH_TOTAL_SIZE (2 * 1024 * 1024) 
#define LFS_FLASH_SECTOR_SIZE (4 * 1024)       
#define LFS_FLASH_PAGE_SIZE (256)             

#define LFS_BLOCK_COUNT (LFS_FLASH_TOTAL_SIZE / LFS_FLASH_SECTOR_SIZE)


int lfs_storage_init(struct lfs_config *cfg);

#endif 