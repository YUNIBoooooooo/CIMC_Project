#include "lfs_port.h"
#include "gd25qxx.h" 
#include <stdio.h>  


static uint8_t lfs_read_buffer[LFS_FLASH_PAGE_SIZE];
static uint8_t lfs_prog_buffer[LFS_FLASH_PAGE_SIZE];
static uint8_t lfs_lookahead_buffer[256 / 8]; 

static int lfs_deskio_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    (void)c; 
    spi_flash_buffer_read(buffer, (block * LFS_FLASH_SECTOR_SIZE) + off, size);
    return LFS_ERR_OK;
}

static int lfs_deskio_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    (void)c; 
    spi_flash_buffer_write((uint8_t *)buffer, (block * LFS_FLASH_SECTOR_SIZE) + off, size);
    return LFS_ERR_OK;
}

static int lfs_deskio_erase(const struct lfs_config *c, lfs_block_t block)
{
    (void)c; 
    spi_flash_sector_erase(block * LFS_FLASH_SECTOR_SIZE);
    return LFS_ERR_OK;
}

static int lfs_deskio_sync(const struct lfs_config *c)
{
    (void)c; 
    
    return LFS_ERR_OK;
}


int lfs_storage_init(struct lfs_config *cfg)
{
    if (!cfg)
        return LFS_ERR_INVAL;

    cfg->context = NULL; 

    
    cfg->read = lfs_deskio_read;
    cfg->prog = lfs_deskio_prog;
    cfg->erase = lfs_deskio_erase;
    cfg->sync = lfs_deskio_sync;

    
    cfg->read_size = LFS_FLASH_PAGE_SIZE;
    cfg->prog_size = LFS_FLASH_PAGE_SIZE;
    cfg->block_size = LFS_FLASH_SECTOR_SIZE;
    cfg->block_count = LFS_BLOCK_COUNT;
    cfg->cache_size = LFS_FLASH_PAGE_SIZE;
    cfg->lookahead_size = sizeof(lfs_lookahead_buffer) * 8; 
    cfg->block_cycles = 500;

    cfg->read_buffer = lfs_read_buffer;
    cfg->prog_buffer = lfs_prog_buffer;
    cfg->lookahead_buffer = lfs_lookahead_buffer;

    cfg->name_max = 0; 
    cfg->file_max = 0; 
    cfg->attr_max = 0; 

    return LFS_ERR_OK;
}


