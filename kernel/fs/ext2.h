#ifndef EXT2_H
#define EXT2_H

#include <stdint.h>
#include "vfs.h"

// ext2 magic number
#define EXT2_MAGIC 0xEF53

// inode types
#define EXT2_S_IFREG  0x8000  // regular file
#define EXT2_S_IFDIR  0x4000  // directory
#define EXT2_S_IFLNK  0xA000  // symbolic link

// root inode is always inode 2
#define EXT2_ROOT_INODE 2

// superblock — lives at byte offset 1024 from start of disk
typedef struct
{
    uint32_t s_inodes_count;       // total inodes
    uint32_t s_blocks_count;       // total blocks
    uint32_t s_r_blocks_count;     // reserved blocks
    uint32_t s_free_blocks_count;  // free blocks
    uint32_t s_free_inodes_count;  // free inodes
    uint32_t s_first_data_block;   // first data block (0 for 1KB blocks, 1 for others)
    uint32_t s_log_block_size;     // log2(block size) - 10
    uint32_t s_log_frag_size;      // log2(fragment size) - 10
    uint32_t s_blocks_per_group;   // blocks per block group
    uint32_t s_frags_per_group;    // fragments per block group
    uint32_t s_inodes_per_group;   // inodes per block group
    uint32_t s_mtime;              // last mount time
    uint32_t s_wtime;              // last write time
    uint16_t s_mnt_count;          // mount count
    uint16_t s_max_mnt_count;      // max mount count before fsck
    uint16_t s_magic;              // magic number 0xEF53
    uint16_t s_state;              // filesystem state
    uint16_t s_errors;             // error handling
    uint16_t s_minor_rev_level;    // minor revision
    uint32_t s_lastcheck;          // last check time
    uint32_t s_checkinterval;      // check interval
    uint32_t s_creator_os;         // creator OS
    uint32_t s_rev_level;          // revision level
    uint16_t s_def_resuid;         // default uid for reserved blocks
    uint16_t s_def_resgid;         // default gid for reserved blocks
    // extended superblock fields (rev 1+)
    uint32_t s_first_ino;          // first non-reserved inode
    uint16_t s_inode_size;         // inode size in bytes
    uint16_t s_block_group_nr;     // block group this superblock is in
    uint32_t s_feature_compat;     // compatible features
    uint32_t s_feature_incompat;   // incompatible features
    uint32_t s_feature_ro_compat;  // read-only compatible features
    uint8_t  s_uuid[16];           // filesystem UUID
    char     s_volume_name[16];    // volume name
    char     s_last_mounted[64];   // last mount path
    uint32_t s_algo_bitmap;        // compression algorithm
    uint8_t  s_prealloc_blocks;    // blocks to preallocate for files
    uint8_t  s_prealloc_dir_blocks;// blocks to preallocate for dirs
    uint16_t s_padding;
    uint8_t  s_reserved[820];      // pad to 1024 bytes
} __attribute__((packed)) ext2_superblock_t;

// block group descriptor
typedef struct
{
    uint32_t bg_block_bitmap;      // block address of block bitmap
    uint32_t bg_inode_bitmap;      // block address of inode bitmap
    uint32_t bg_inode_table;       // block address of inode table
    uint16_t bg_free_blocks_count; // free blocks in group
    uint16_t bg_free_inodes_count; // free inodes in group
    uint16_t bg_used_dirs_count;   // directories in group
    uint16_t bg_pad;
    uint8_t  bg_reserved[12];
} __attribute__((packed)) ext2_group_desc_t;

// inode
typedef struct
{
    uint16_t i_mode;        // file type and permissions
    uint16_t i_uid;         // owner user id
    uint32_t i_size;        // file size in bytes
    uint32_t i_atime;       // last access time
    uint32_t i_ctime;       // creation time
    uint32_t i_mtime;       // last modification time
    uint32_t i_dtime;       // deletion time
    uint16_t i_gid;         // group id
    uint16_t i_links_count; // hard link count
    uint32_t i_blocks;      // 512-byte blocks used
    uint32_t i_flags;       // file flags
    uint32_t i_osd1;        // OS specific value
    uint32_t i_block[15];   // block pointers
                             // [0-11]  direct blocks
                             // [12]    single indirect
                             // [13]    double indirect
                             // [14]    triple indirect
    uint32_t i_generation;  // file version
    uint32_t i_file_acl;    // file ACL
    uint32_t i_dir_acl;     // directory ACL
    uint32_t i_faddr;       // fragment address
    uint8_t  i_osd2[12];    // OS specific value 2
} __attribute__((packed)) ext2_inode_t;

// directory entry
typedef struct
{
    uint32_t inode;     // inode number (0 = unused entry)
    uint16_t rec_len;   // total size of this entry including name
    uint8_t  name_len;  // length of the name
    uint8_t  file_type; // file type (1=file, 2=dir, 7=symlink)
    char     name[];    // filename (not null terminated)
} __attribute__((packed)) ext2_dirent_t;

// ext2 filesystem instance
typedef struct
{
    ext2_superblock_t superblock;
    uint32_t          block_size;
    uint32_t          groups_count;
    uint32_t          inodes_per_group;
    uint32_t          blocks_per_group;
    uint32_t          inode_size;
} ext2_fs_t;

vfs_node_t *ext2_init();

#endif