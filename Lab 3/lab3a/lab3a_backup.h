typedef struct metadata_t {
        int offset;
        int value;
        int size;
        char *format;
} metadata_t;
/* Info Structures */
typedef struct pos_info {
        char* name;
        char* format;
        int offset;
        int size;
} pos_info;

static pos_info superblock[] = {
        {"magic number",           "%x", 56, 2}, // s_magic
        {"total number of inodes", "%d",  0, 4}, // s_inodes_count
        {"total number of blocks", "%d",  4, 4}, // s_blocks_count
        {"block size",             "%d", 24, 4}, // s_log_block_size
        {"fragment size",          "%d", 28, 4}, // s_log_frag_size
        {"blocks per group",       "%d", 32, 4}, // s_blocks_per_group
        {"inodes per group",       "%d", 40, 4}, // s_inodes_per_group
        {"fragments per group",    "%d", 40, 4}, // s_frags_per_group
        {"first data block",       "%d", 20, 4}, // s_first_data_block
};

static pos_info group_descriptor[] = {
        {"number of contained blocks", "%d", BAD, BAD},
        {"number of free blocks",      "%d", 12, 2}, // bg_free_blocks_count
        {"number of free inodes",      "%d", 14, 2}, // bg_free_inodes_count
        {"number of directories",      "%d", 16, 2}, // bg_used_dirs_count
        {"free inode bitmap block",    "%x",  4, 4}, // bg_inode_bitmap
        {"free block bitmap block",    "%x",  0, 4}, // bg_block_bitmap
        {"inode table (start) block",  "%x",  8, 4}  // bg_inode_table
};

static pos_info free_bitmap_entry[] = {
        {"block number of the map", "%x", 1, 1},
        {"entry number",            "%d", 1, 1}
};

static pos_info inode[] = {
        {"inode number",      "%d", 1, 1},
        {"file type",         "%s", 1, 1},
        {"mode",              "%o", 1, 1},
        {"owner",             "%d", 1, 1},
        {"group",             "%d", 1, 1},
        {"link count",        "%d", 1, 1},
        {"creation time",     "%x", 1, 1},
        {"modification time", "%x", 1, 1},
        {"access time",       "%x", 1, 1},
        {"file size",         "%d", 1, 1},
        {"number of blocks",  "%d", 1, 1},
        {"block pointer 1",   "%x", 1, 1},
        {"block pointer 2",   "%x", 1, 1},
        {"block pointer 3",   "%x", 1, 1},
        {"block pointer 4",   "%x", 1, 1},
        {"block pointer 5",   "%x", 1, 1},
        {"block pointer 6",   "%x", 1, 1},
        {"block pointer 7",   "%x", 1, 1},
        {"block pointer 8",   "%x", 1, 1},
        {"block pointer 9",   "%x", 1, 1},
        {"block pointer 10",  "%x", 1, 1},
        {"block pointer 11",  "%x", 1, 1},
        {"block pointer 12",  "%x", 1, 1},
        {"block pointer 13",  "%x", 1, 1},
        {"block pointer 14",  "%x", 1, 1},
        {"block pointer 15",  "%x", 1, 1}
};

static pos_info directory_entry[] = {
        {"parent inode number", "%d", 1, 1},
        {"entry number",        "%d", 1, 1},
        {"entry length",        "%d", 1, 1},
        {"name length",         "%d", 1, 1},
        {"link inode number",   "%d", 1, 1},
        {"name",                "%s", 1, 1}
}

static pos_info indirect_block_entry[] = {
        {"block number of containing block", "%x", 1, 1},
        {"entry number",                     "%d", 1, 1},
        {"pointer value",                    "%x", 1, 1}
}
/*
   typedef struct superblock_t {
        metadata_t magicNumber    = {56, 0, 2, "%x"}; // s_magic
        metadata_t inodeCount     = {0,  0, 4, "%d"}; // s_inodes_count
        metadata_t blockCount     = {4,  0, 4, "%d"}; // s_blocks_count
        metadata_t blockSize      = {24, 0, 4, "%d"}; // s_log_block_size
        metadata_t fragSize       = {28, 0, 4, "%d"}; // s_log_frag_size
        metadata_t blocksPerGroup = {32, 0, 4, "%d"}; // s_blocks_per_group
        metadata_t inodesPerGroup = {40, 0, 4, "%d"}; // s_inodes_per_group
        metadata_t fragsPerGroup  = {36, 0, 4, "%d"}; // s_frags_per_group
        metadata_t firstDataBlock = {20, 0, 4, "%d"}; // s_first_data_block
   } superblock_t;
 */

struct diskorg_t {
        metadata_t contents[];
}
superblock_t = {
        {
                {"magic number",           "%x", 56, 2},// s_magic
                {"total number of inodes", "%d",  0, 4},// s_inodes_count
                {"total number of blocks", "%d",  4, 4},// s_blocks_count
                {"block size",             "%d", 24, 4},// s_log_block_size
                {"fragment size",          "%d", 28, 4},// s_log_frag_size
                {"blocks per group",       "%d", 32, 4},// s_blocks_per_group
                {"inodes per group",       "%d", 40, 4},// s_inodes_per_group
                {"fragments per group",    "%d", 40, 4},// s_frags_per_group
                {"first data block",       "%d", 20, 4},// s_first_data_block
        }
},
        group_descriptor_t {
        {
                {"number of contained blocks", "%d", BAD, BAD},
                {"number of free blocks",      "%d", 12, 2},// bg_free_blocks_count
                {"number of free inodes",      "%d", 14, 2},// bg_free_inodes_count
                {"number of directories",      "%d", 16, 2},// bg_used_dirs_count
                {"free inode bitmap block",    "%x",  4, 4},// bg_inode_bitmap
                {"free block bitmap block",    "%x",  0, 4},// bg_block_bitmap
                {"inode table (start) block",  "%x",  8, 4}// bg_inode_table
        }
}
// We can include more default implementations here that diskorg_t objects can be assigned to
;

typedef struct diskorg_t diskorg_t;


// Would need to call the following functions whenever we want to use/instantiate a specific disk org, given the defaults above.
// These should be called either in a setup/init function at the beginning of execution (at least for those things that will always be known),
//		OR, during analysis of the drive when we find more of these structures necessary.
// May want to also automatically malloc space inside these functions for ease of use later too (or not lol)
diskorg_t*
init_superblock_t (diskorg_t *in) {
        in = malloc(sizeof(superblock_t));
        *in = superblock_t;
        return in;
}

diskorg_t*
init_group_descriptor_t (diskorg_t *in) {
        in = malloc(sizeof(group_descriptor_t));
        *in = group_descriptor_t;
        return in;
}
