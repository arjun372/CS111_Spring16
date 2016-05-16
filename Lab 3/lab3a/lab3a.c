/**
    UCLA CS 111 - Spring '16
    Lab 3A - File System Dump
    Rahul Malavalli - 204429252
    Arjun Arjun     - 504078752
 **/

#define FILE_INDIRECT_BLOCK_ENTRIES  "indirect.csv"
#define FILE_DIRECTORY_ENTRIES       "directory.csv"
#define FILE_GROUP_DESCRIPTOR        "group.csv"
#define FILE_FREE_BITMAPS            "bitmap.csv"
#define FILE_SUPERBLOCK              "super.csv"
#define FILE_INODES                  "inodes.csv"

#define CSV_WRITE_FLAGS  O_WRONLY|O_CREAT|O_TRUNC
#define FILE_MODE 0664
#define BAD         -1
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>  /* for fprintf used in debug_log */
#include <stdint.h>
#include <getopt.h> /* Argument Options parse headers */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "lab3a.h"

/* global diskorg_t variables */
/** Single pointer if there is only ONE data structure (like superblock)**/
/** Double pointer if there is an array of data structures **/
static SuperBlock_t *superblock_data;
static GroupDescriptor_t **group_descriptor_table;

/* option-specific variables */
static int VERBOSE = 0;
static struct option long_options[] = {
        {"verbose",          no_argument,  &VERBOSE,   1}
};

/* Static function declarations */
static int init_GroupDescriptorTable_info(GroupDescriptor_t **groupDescriptorTable);
static void debug_log(const int opt_index, char **optarg, const int argc);
static int fill_superblock(SuperBlock_t *blockToFill, int fd);
static int fill_block(Block_t *blockToFill, int fd);
static SuperBlock_t *init_superblock_info();
static void writeCSV_superblock();

int main (int argc, char **argv)
{

        uint32_t i;
        int FD, opt_index;
        /* Read --verbose option if it was passed */
        while(getopt_long_only(argc, argv, "", long_options, &opt_index) != -1)
                continue;

        if(argc <= (VERBOSE ? 2 : 1)) {
                fprintf(stderr, "FATAL: no file passed as argument!\n");
                exit(1);
        }

        char *TargetFile  = argv[VERBOSE ? 2 : 1];
        FD = open(TargetFile, O_RDONLY, FILE_MODE);
        if(FD < 0) {
                fprintf(stderr, "FATAL(%d): %s\n", errno, strerror(errno));
                exit(1);
        } else if(VERBOSE) fprintf(stderr, "Selecting file '%s'\n", TargetFile);

        /* Process superblock information */
        superblock_data = init_superblock_info();
        if(fill_superblock(superblock_data, FD))
                writeCSV_superblock();

        /* Read into groupDescriptor table for each block group */
        uint32_t nGroupDescriptor = init_GroupDescriptorTable_info(group_descriptor_table);

        for(i = 0; i < nGroupDescriptor; i++)
                free(group_descriptor_table[i]);   // free each GD in gdTable
        free(group_descriptor_table);              // free gdTable
        free(superblock_data);                     // free superblock data
        close(FD);                                 // close TargetFile
        exit(0);
}

static void writeCSV_superblock() {
        int fd = open(FILE_SUPERBLOCK, CSV_WRITE_FLAGS, FILE_MODE);
        if(fd < 0) {
                fprintf(stderr, "FATAL(%d): %s\n", errno, strerror(errno));
                exit(1);
        } else if(VERBOSE) fprintf(stderr, "Writing CSV: '%s'\n", FILE_SUPERBLOCK);

        uint32_t i;
        for(i = 0; i < (superblock_data->nDataObjects); i++) {
                dprintf(fd, superblock_data->dataObjects[i].format, superblock_data->dataObjects[i].value);
                dprintf(fd, (i == (superblock_data->nDataObjects - 1)) ? "\n" : ",");
        }
        close(fd);
}

/* Fills the given data structure based on the values it stores */
static int fill_superblock(SuperBlock_t *toFill, const int fd) {

        /* If filling the block from disk failed, then return 0 */
        if(!fill_block(toFill, fd))
                return 0;

        /** Now do SuperBlock_t specific actions */
        uint32_t correct_BlockSize   = 1024 << toFill->dataObjects[3].value;
        uint32_t correct_FragSize    = (toFill->dataObjects[4].value > 0) ? (1024 << toFill->dataObjects[4].value) : (1024 >> -(toFill->dataObjects[4].value));
        toFill->dataObjects[3].value = correct_BlockSize;
        toFill->dataObjects[4].value = correct_FragSize;

        /* Print it out if VERBOSE */
        if(VERBOSE) {
                uint32_t i;
                for(i = 0; i < (toFill->nDataObjects); i++) {
                        fprintf(stderr, toFill->dataObjects[i].format, toFill->dataObjects[i].value);
                        fprintf(stderr, (i == (toFill->nDataObjects - 1)) ? "\n" : ",");
                }
        }
        return 1;
}

/* Fills the given data structure based on the values it stores */
static int fill_block(Block_t *toFill, const int fd) {
        uint32_t i;
        int bytesRead = 0;
        for(i = 0; i < (toFill->nDataObjects); i++)
                bytesRead += pread(fd, &(toFill->dataObjects[i].value), toFill->dataObjects[i].size, toFill->dataObjects[i].offset);
        return bytesRead;
}

/* Reads the disk superblock, stores it in SuperBlock_t data structure,
 * and returns a pointer to the newly created data structure */
static SuperBlock_t *init_superblock_info() {

        /* allocate memory for SuperBlock_t data structure */
        SuperBlock_t *mSuperBlock = (SuperBlock_t *) malloc(sizeof(SuperBlock_t));
        if(mSuperBlock == NULL) {
                fprintf(stderr, "FATAL:: Unable to allocate memory for reading superblock\n");
                exit(1);
        }

        // allocate memory for N MetaData_t objects that do within SuperBlock_t
        MetaData_t *mDataObjects = (MetaData_t *) malloc(sizeof(MetaData_t) * SUPERBLOCK_FIELDS);
        if(mDataObjects == NULL) {
                fprintf(stderr, "FATAL:: Unable to allocate memory for reading superblock\n");
                free(mSuperBlock);
                exit(1);
        }

        /* assign this new data structure to superblock */
        mSuperBlock->nDataObjects = SUPERBLOCK_FIELDS;
        mSuperBlock->dataObjects  = mDataObjects;

        MetaData_t magicNumber    = {SUPERBLOCK_OFF + 56, 0, 2, "%x"}; // s_magic
        MetaData_t inodeCount     = {SUPERBLOCK_OFF + 0,  0, 4, "%d"}; // s_inodes_count
        MetaData_t blockCount     = {SUPERBLOCK_OFF + 4,  0, 4, "%d"}; // s_blocks_count
        MetaData_t blockSize      = {SUPERBLOCK_OFF + 24, 0, 4, "%d"}; // s_log_block_size
        MetaData_t fragSize       = {SUPERBLOCK_OFF + 28, 0, 4, "%d"}; // s_log_frag_size
        MetaData_t blocksPerGroup = {SUPERBLOCK_OFF + 32, 0, 4, "%d"}; // s_blocks_per_group
        MetaData_t inodesPerGroup = {SUPERBLOCK_OFF + 40, 0, 4, "%d"}; // s_inodes_per_group
        MetaData_t fragsPerGroup  = {SUPERBLOCK_OFF + 36, 0, 4, "%d"}; // s_frags_per_group
        MetaData_t firstDataBlock = {SUPERBLOCK_OFF + 20, 0, 4, "%d"}; // s_first_data_block

        /* populate the dataObjects */
        // TODO : Do we know a better way to allocating the @params above?
        mSuperBlock->dataObjects[0] = magicNumber;
        mSuperBlock->dataObjects[1] = inodeCount;
        mSuperBlock->dataObjects[2] = blockCount;
        mSuperBlock->dataObjects[3] = blockSize;
        mSuperBlock->dataObjects[4] = fragSize;
        mSuperBlock->dataObjects[5] = blocksPerGroup;
        mSuperBlock->dataObjects[6] = inodesPerGroup;
        mSuperBlock->dataObjects[7] = fragsPerGroup;
        mSuperBlock->dataObjects[8] = firstDataBlock;

        return mSuperBlock;
}

static int init_GroupDescriptorTable_info(GroupDescriptor_t **groupDescriptorTable) {

        /* Calculate how many GroupDescriptorTable members we will need based on number of block groups */
        uint32_t i;
        uint32_t blockCount     = superblock_data->dataObjects[2].value;
        uint32_t blocksPerGroup = superblock_data->dataObjects[5].value;
        uint32_t nBlockGroups   = (blockCount + blocksPerGroup - 1) / blocksPerGroup;

        /* allocate memory for GroupDescriptor_Table members */
        groupDescriptorTable = (GroupDescriptor_t **) malloc(sizeof(GroupDescriptor_t *) * nBlockGroups);
        if(groupDescriptorTable == NULL) goto MEM_ERR;

        /* The block group descriptor table starts on the first block following the superblock.
         * This would be the third block on a 1KiB block file system, or the second block for
         * 2KiB and larger block file systems. Shadow copies of the block	group descriptor table
         * are also stored with every copy of the superblock. */

        uint32_t blockSize           = superblock_data->dataObjects[3].value;
        uint32_t gdTable_blockOffset = (blockSize <= 1024) ? 2 : 1;
        uint32_t startOffset         = blockSize * gdTable_blockOffset;

        uint32_t bgd_size            = 32;
        uint32_t bgdTable_size       = bgd_size * nBlockGroups;

        /* Depending on how many block groups are defined, this table can require multiple blocks
         * of storage. Always refer to the superblock in case of doubt. */

        /* allocate memory for each group_descriptor */
        for(i = 0; i < nBlockGroups; i++) {
                groupDescriptorTable[i] = (GroupDescriptor_t *) malloc(sizeof(GroupDescriptor_t));
                if(groupDescriptorTable[i] == NULL) goto MEM_ERR;

                /* initialize metadata for each group_descriptor */
                MetaData_t *mDataObjects = (MetaData_t *) malloc(sizeof(MetaData_t) * GROUPDESCRIPTOR_FIELDS);
                if(mDataObjects == NULL) goto MEM_ERR;

                groupDescriptorTable[i]->nDataObjects = GROUPDESCRIPTOR_FIELDS;
                groupDescriptorTable[i]->dataObjects  = mDataObjects;

                uint32_t OFFSET = startOffset + (i * bgd_size);

                MetaData_t numContainedBlocks   = {OFFSET +    0, 0, 4, "%d"};  // will not be read by pread!
                MetaData_t numFreeBlocks        = {OFFSET +   12, 0, 2, "%d"};
                MetaData_t numFreeInodes        = {OFFSET +   14, 0, 2, "%d"};
                MetaData_t numUsedDirs          = {OFFSET +   16, 0, 2, "%d"};
                MetaData_t freeInodeBMP         = {OFFSET +    4, 0, 4, "%x"};
                MetaData_t freeBlockBMP         = {OFFSET +    0, 0, 4, "%x"};
                MetaData_t inodeTableStartBlock = {OFFSET +    8, 0, 4, "%x"};

                groupDescriptorTable[i]->dataObjects[0] = numContainedBlocks;
                groupDescriptorTable[i]->dataObjects[1] = numFreeBlocks;
                groupDescriptorTable[i]->dataObjects[2] = numFreeInodes;
                groupDescriptorTable[i]->dataObjects[3] = numUsedDirs;
                groupDescriptorTable[i]->dataObjects[4] = freeInodeBMP;
                groupDescriptorTable[i]->dataObjects[5] = freeBlockBMP;
                groupDescriptorTable[i]->dataObjects[6] = inodeTableStartBlock;
        }

        if(VERBOSE) {
                fprintf(stderr, "nBlockGroups: %d\n", nBlockGroups);
                fprintf(stderr, "BlockSize:: %d\n", blockSize);
                fprintf(stderr, "gdTable_blockOffset:: %d\n",gdTable_blockOffset);
                fprintf(stderr, "gdTable_byteStartOffset:: %d\n", startOffset);
        }

        return nBlockGroups;
MEM_ERR:
        fprintf(stderr, "FATAL:: Unable to allocate memory for reading group descriptors \n");
        exit(1);
}

/* if --VERBOSE is passed, logs to stdout */
static void debug_log(const int opt_index, char **optarg, const int argc) {
        fprintf(stderr,"--%s", long_options[opt_index].name);
        int i;
        for(i = 0; i < argc; i++)
                fprintf(stderr," %s", optarg[i]);
        fprintf(stderr,"\n");
}
