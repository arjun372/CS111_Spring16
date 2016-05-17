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
        /* ------- These are not part of EXT2 structs -------- */
        int nInodes;
        int inodeStart;      // first inode for this block group
        int allocated_space; // for inodePtr
        int nAllocated;
        /*----*/

        // Default members
        uint32_t nDataObjects;
        struct metadata dataObjects[];
};


#define FREE_BITMAP_FIELD       2
struct free_bitmap_entry {
        uint32_t nDataObjects;
        struct metadata dataObjects[];
};

typedef struct metadata MetaData_t;
typedef struct superblock Block_t;
typedef struct superblock SuperBlock_t;
typedef struct group_descriptor GroupDescriptor_t;
typedef struct free_bitmap_entry FreeBitmapEntry_t;

static const SuperBlock_t DEFAULT_SUPERBLOCK_T = {
        SUPERBLOCK_FIELDS, // nDataObjects
        {
                {SUPERBLOCK_OFF + 56, 0, 2, "%x"}, // s_magic
                {SUPERBLOCK_OFF + 0,  0, 4, "%d"},// s_inodes_count
                {SUPERBLOCK_OFF + 4,  0, 4, "%d"},// s_blocks_count
                {SUPERBLOCK_OFF + 24, 0, 4, "%d"}, // s_log_block_size
                {SUPERBLOCK_OFF + 28, 0, 4, "%d"}, // s_log_frag_size
                {SUPERBLOCK_OFF + 32, 0, 4, "%d"}, // s_blocks_per_group
                {SUPERBLOCK_OFF + 40, 0, 4, "%d"}, // s_inodes_per_group
                {SUPERBLOCK_OFF + 36, 0, 4, "%d"}, // s_frags_per_group
                {SUPERBLOCK_OFF + 20, 0, 4, "%d"}, // s_first_data_block
        } // dataObjects
};

static const GroupDescriptor_t DEFAULT_GROUP_DESCR_T = {
        0, 0, 0, 0, /*-- UNKNOWN. TODO: determine use, if any -*/
        GROUPDESCRIPTOR_FIELDS, // nDataObjects
        {
                {0, 0, 4, "%d"}, // will not be read by pread!
                {12, 0, 2, "%d"},
                {14, 0, 2, "%d"},
                {16, 0, 2, "%d"},
                {4, 0, 4, "%x"},
                {0, 0, 4, "%x"},
                {8, 0, 4, "%x"}
        } // dataObjects
};

static const FreeBitmapEntry_t DEFAULT_FREE_BITMAP_ENTRY_T = {
        FREE_BITMAP_FIELD,      // nDataObjects
        {
                {0, 0, 0, "%x"},
                {0, 0, 0, "%d"}
        }
};
