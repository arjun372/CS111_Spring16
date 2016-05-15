struct metadata {
        int offset;
        int value;
        int size;
        char format[2]; // formatting string doesn't need to be longer than 2
                        // this saves a lot of malloc and free calls!
};

struct superblock {
        struct metadata magicNumber;    // s_magic
        struct metadata inodeCount;     // s_inodes_count
        struct metadata blockCount;     // s_blocks_count
        struct metadata blockSize;      // s_log_block_size
        struct metadata fragSize;       // s_log_frag_size
        struct metadata blocksPerGroup; // s_blocks_per_group
        struct metadata inodesPerGroup; // s_inodes_per_group
        struct metadata fragsPerGroup;  // s_frags_per_group
        struct metadata firstDataBlock; // s_first_data_block
};

typedef struct metadata MetaData_t;
typedef struct superblock SuperBlock_t;

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
