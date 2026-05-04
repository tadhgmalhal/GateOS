#include "ext2.h"
#include "vfs.h"
#include "../drivers/disk_cache.h"
#include "../lib/string.h"
#include "../lib/kprintf.h"
#include "../mm/kheap.h"

static ext2_fs_t fs;

// read a block from disk into buffer
static void ext2_read_block(uint32_t block, void *buffer)
{
    uint32_t sectors_per_block = fs.block_size / 512;
    uint32_t lba               = block * sectors_per_block;

    uint8_t *buf = (uint8_t *)buffer;
    for (uint32_t i = 0; i < sectors_per_block; i++)
    {
        disk_cache_read(lba + i, buf + i * 512);
    }
}

// write a block to disk
static void ext2_write_block(uint32_t block, const void *buffer)
{
    uint32_t sectors_per_block = fs.block_size / 512;
    uint32_t lba               = block * sectors_per_block;

    const uint8_t *buf = (const uint8_t *)buffer;
    for (uint32_t i = 0; i < sectors_per_block; i++)
    {
        disk_cache_write(lba + i, buf + i * 512);
    }
}

// read an inode from disk
static void ext2_read_inode(uint32_t inode_num, ext2_inode_t *inode)
{
    uint32_t group   = (inode_num - 1) / fs.inodes_per_group;
    uint32_t index   = (inode_num - 1) % fs.inodes_per_group;
    uint32_t offset  = index * fs.inode_size;

    // read group descriptor to find inode table
    uint32_t gd_block  = (fs.block_size == 1024) ? 2 : 1;
    uint8_t *gd_buf    = (uint8_t *)kmalloc(fs.block_size);
    ext2_read_block(gd_block, gd_buf);

    ext2_group_desc_t *gd = (ext2_group_desc_t *)(gd_buf + group * sizeof(ext2_group_desc_t));
    uint32_t inode_table  = gd->bg_inode_table;
    kfree(gd_buf);

    // calculate which block and offset within that block the inode is at
    uint32_t block_offset = offset / fs.block_size;
    uint32_t byte_offset  = offset % fs.block_size;

    uint8_t *block_buf = (uint8_t *)kmalloc(fs.block_size);
    ext2_read_block(inode_table + block_offset, block_buf);
    memcpy(inode, block_buf + byte_offset, sizeof(ext2_inode_t));
    kfree(block_buf);
}

// get the nth data block of an inode
static uint32_t ext2_get_block(ext2_inode_t *inode, uint32_t block_index)
{
    uint32_t ptrs_per_block = fs.block_size / 4;

    if (block_index < 12)
    {
        // direct block
        return inode->i_block[block_index];
    }

    block_index -= 12;

    if (block_index < ptrs_per_block)
    {
        // single indirect
        uint32_t *indirect = (uint32_t *)kmalloc(fs.block_size);
        ext2_read_block(inode->i_block[12], indirect);
        uint32_t result = indirect[block_index];
        kfree(indirect);
        return result;
    }

    block_index -= ptrs_per_block;

    if (block_index < ptrs_per_block * ptrs_per_block)
    {
        // double indirect
        uint32_t *indirect1 = (uint32_t *)kmalloc(fs.block_size);
        ext2_read_block(inode->i_block[13], indirect1);

        uint32_t idx1 = block_index / ptrs_per_block;
        uint32_t idx2 = block_index % ptrs_per_block;

        uint32_t *indirect2 = (uint32_t *)kmalloc(fs.block_size);
        ext2_read_block(indirect1[idx1], indirect2);

        uint32_t result = indirect2[idx2];
        kfree(indirect1);
        kfree(indirect2);
        return result;
    }

    // triple indirect — not implemented yet
    kprintf("ext2: triple indirect blocks not supported\n");
    return 0;
}

// read bytes from an inode
static uint32_t ext2_read_inode_data(ext2_inode_t *inode, uint32_t offset,
                                      uint32_t size, uint8_t *buffer)
{
    if (offset >= inode->i_size)
    {
        return 0;
    }

    uint32_t to_read = size;
    if (offset + size > inode->i_size)
    {
        to_read = inode->i_size - offset;
    }

    uint32_t bytes_read  = 0;
    uint8_t *block_buf   = (uint8_t *)kmalloc(fs.block_size);

    while (bytes_read < to_read)
    {
        uint32_t block_index  = (offset + bytes_read) / fs.block_size;
        uint32_t block_offset = (offset + bytes_read) % fs.block_size;
        uint32_t block_num    = ext2_get_block(inode, block_index);

        if (block_num == 0)
        {
            break;
        }

        ext2_read_block(block_num, block_buf);

        uint32_t available = fs.block_size - block_offset;
        uint32_t chunk     = to_read - bytes_read;
        if (chunk > available)
        {
            chunk = available;
        }

        memcpy(buffer + bytes_read, block_buf + block_offset, chunk);
        bytes_read += chunk;
    }

    kfree(block_buf);
    return bytes_read;
}

// VFS read function for ext2 files
static uint32_t ext2_vfs_read(vfs_node_t *node, uint32_t offset,
                               uint32_t size, uint8_t *buffer)
{
    ext2_inode_t inode;
    ext2_read_inode(node->inode, &inode);
    return ext2_read_inode_data(&inode, offset, size, buffer);
}

// VFS readdir for ext2 directories
static vfs_dirent_t *ext2_vfs_readdir(vfs_node_t *node, uint32_t index)
{
    ext2_inode_t inode;
    ext2_read_inode(node->inode, &inode);

    uint8_t *block_buf = (uint8_t *)kmalloc(fs.block_size);
    static vfs_dirent_t dirent;

    uint32_t offset  = 0;
    uint32_t count   = 0;
    uint32_t entries = 0;

    while (offset < inode.i_size)
    {
        uint32_t block_index  = offset / fs.block_size;
        uint32_t block_num    = ext2_get_block(&inode, block_index);

        if (block_num == 0)
        {
            break;
        }

        ext2_read_block(block_num, block_buf);

        uint32_t block_offset = 0;
        while (block_offset < fs.block_size)
        {
            ext2_dirent_t *de = (ext2_dirent_t *)(block_buf + block_offset);

            if (de->rec_len == 0)
            {
                break;
            }

            if (de->inode != 0)
            {
                if (count == index)
                {
                    memcpy(dirent.name, de->name, de->name_len);
                    dirent.name[de->name_len] = '\0';
                    dirent.inode = de->inode;
                    kfree(block_buf);
                    return &dirent;
                }
                count++;
            }

            block_offset += de->rec_len;
            entries++;
        }

        offset += fs.block_size;
    }

    kfree(block_buf);
    return 0;
}

// VFS finddir for ext2 directories
static vfs_node_t *ext2_vfs_finddir(vfs_node_t *node, const char *name)
{
    ext2_inode_t inode;
    ext2_read_inode(node->inode, &inode);

    uint8_t *block_buf = (uint8_t *)kmalloc(fs.block_size);
    uint32_t offset    = 0;

    while (offset < inode.i_size)
    {
        uint32_t block_index = offset / fs.block_size;
        uint32_t block_num   = ext2_get_block(&inode, block_index);

        if (block_num == 0)
        {
            break;
        }

        ext2_read_block(block_num, block_buf);

        uint32_t block_offset = 0;
        while (block_offset < fs.block_size)
        {
            ext2_dirent_t *de = (ext2_dirent_t *)(block_buf + block_offset);

            if (de->rec_len == 0)
            {
                break;
            }

            if (de->inode != 0 && de->name_len == strlen(name) &&
                strncmp(de->name, name, de->name_len) == 0)
            {
                // found it — read its inode and build a VFS node
                ext2_inode_t child_inode;
                ext2_read_inode(de->inode, &child_inode);

                vfs_node_t *child = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
                memset(child, 0, sizeof(vfs_node_t));

                strncpy(child->name, name, VFS_NAME_MAX - 1);
                child->name[VFS_NAME_MAX - 1] = '\0';
                child->inode = de->inode;
                child->size  = child_inode.i_size;

                uint16_t type = child_inode.i_mode & 0xF000;
                if (type == EXT2_S_IFREG)
                {
                    child->type  = VFS_FILE;
                    child->read  = ext2_vfs_read;
                }
                else if (type == EXT2_S_IFDIR)
                {
                    child->type    = VFS_DIRECTORY;
                    child->readdir = ext2_vfs_readdir;
                    child->finddir = ext2_vfs_finddir;
                }

                kfree(block_buf);
                return child;
            }

            block_offset += de->rec_len;
        }

        offset += fs.block_size;
    }

    kfree(block_buf);
    return 0;
}

vfs_node_t *ext2_init()
{
    // read superblock from byte offset 1024 = sector 2
    uint8_t sb_buf[1024];
    disk_cache_read(2, sb_buf);
    disk_cache_read(3, sb_buf + 512);
    memcpy(&fs.superblock, sb_buf, sizeof(ext2_superblock_t));

    if (fs.superblock.s_magic != EXT2_MAGIC)
    {
        kprintf("ext2: bad magic number (got %x, expected %x)\n",
                fs.superblock.s_magic, EXT2_MAGIC);
        return 0;
    }

    fs.block_size      = 1024 << fs.superblock.s_log_block_size;
    fs.inodes_per_group = fs.superblock.s_inodes_per_group;
    fs.blocks_per_group = fs.superblock.s_blocks_per_group;
    fs.inode_size      = (fs.superblock.s_rev_level >= 1)
                         ? fs.superblock.s_inode_size
                         : 128;
    fs.groups_count    = (fs.superblock.s_blocks_count +
                          fs.blocks_per_group - 1) / fs.blocks_per_group;

    kprintf("ext2: filesystem mounted.\n");
    kprintf("  block size:    %d bytes\n", fs.block_size);
    kprintf("  total blocks:  %d\n", fs.superblock.s_blocks_count);
    kprintf("  total inodes:  %d\n", fs.superblock.s_inodes_count);
    kprintf("  block groups:  %d\n", fs.groups_count);
    kprintf("  inode size:    %d bytes\n", fs.inode_size);

    // build root VFS node from inode 2
    ext2_inode_t root_inode;
    ext2_read_inode(EXT2_ROOT_INODE, &root_inode);

    vfs_node_t *root = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
    memset(root, 0, sizeof(vfs_node_t));
    strncpy(root->name, "/", VFS_NAME_MAX - 1);
    root->inode   = EXT2_ROOT_INODE;
    root->size    = root_inode.i_size;
    root->type    = VFS_DIRECTORY;
    root->readdir = ext2_vfs_readdir;
    root->finddir = ext2_vfs_finddir;

    return root;
}