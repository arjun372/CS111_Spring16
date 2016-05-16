struct metadata {
        uint32_t offset;
        uint32_t value;
        uint32_t size;
        char format[2]; // formatting string doesn't need to be longer than 2
                        // this saves a lot of malloc and free calls!
};

#define SUPERBLOCK_SIZE    1024
#define SUPERBLOCK_OFF     1024
#define SUPERBLOCK_FIELDS     9
struct superblock {
        uint32_t nDataObjects;
        struct metadata dataObjects[];
};

#define GROUPDESCRIPTOR_FIELDS    7
struct group_descriptor {
        struct metadata *dataObjects;
        uint32_t nDataObjects;

        /* ------- These are not part of EXT2 structs -------- */
        int nInodes;
        int inodeStart;      // first inode for this block group
        int allocated_space; // for inodePtr
        int nAllocated;
};

typedef struct metadata MetaData_t;
typedef struct superblock Block_t;
typedef struct superblock SuperBlock_t;
typedef struct group_descriptor GroupDescriptor_t;

static const SuperBlock_t DEFAULT_SUPERBLOCK_T = {
  SUPERBLOCK_FIELDS,  // nDataObjects
  {
    {SUPERBLOCK_OFF + 56, 0, 2, "%x"}, // s_magic
    {SUPERBLOCK_OFF + 0,  0, 4, "%d"}, // s_inodes_count
    {SUPERBLOCK_OFF + 4,  0, 4, "%d"}, // s_blocks_count
    {SUPERBLOCK_OFF + 24, 0, 4, "%d"}, // s_log_block_size
    {SUPERBLOCK_OFF + 28, 0, 4, "%d"}, // s_log_frag_size
    {SUPERBLOCK_OFF + 32, 0, 4, "%d"}, // s_blocks_per_group
    {SUPERBLOCK_OFF + 40, 0, 4, "%d"}, // s_inodes_per_group
    {SUPERBLOCK_OFF + 36, 0, 4, "%d"}, // s_frags_per_group
    {SUPERBLOCK_OFF + 20, 0, 4, "%d"}, // s_first_data_block
  }  // dataObjects
};

// //inode_t inodePtr; TODO :: I still don't understand this
//
// struct metadata nBlocks;         // ??
// struct metadata nFreeBlocks;     // bg_free_blocks_count
// struct metadata nFreeInodes;     //  bg_free_inodes_count
// struct metadata nUserDirs;       // bg_used_dirs_count
// struct metadata inodeMapBlock;   // bg_inode_bitmap
// struct metadata blockMapBlock;   // bg_block_bitmap
// struct metadata inodeTableBlock; // bg_inode_table

// struct superblock_t {
//         metadata_t magicNumber    = {56, 0, 2, "%x"};// s_magic
//         metadata_t inodeCount     = {0,  0, 4, "%d"};// s_inodes_count
//         metadata_t blockCount     = {4,  0, 4, "%d"};// s_blocks_count
//         metadata_t blockSize      = {24, 0, 4, "%d"};// s_log_block_size
//         metadata_t fragSize       = {28, 0, 4, "%d"};// s_log_frag_size
//         metadata_t blocksPerGroup = {32, 0, 4, "%d"}; // s_blocks_per_group
//         metadata_t inodesPerGroup = {40, 0, 4, "%d"}; // s_inodes_per_group
//         metadata_t fragsPerGroup  = {36, 0, 4, "%d"};// s_frags_per_group
//         metadata_t firstDataBlock = {20, 0, 4, "%d"}; // s_first_data_block
// } superblock_t;
