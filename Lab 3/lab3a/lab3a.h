typedef struct metadata_t {
        int offset;
        int value;
        int size;
        char *format;
}

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
};
