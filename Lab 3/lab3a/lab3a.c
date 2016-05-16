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
static int fill_GroupDescriptors(const int fd, GroupDescriptor_t **gdTable, const uint32_t nGDs);
static uint32_t init_GroupDescriptorTable_info(GroupDescriptor_t **groupDescriptorTable);
static int fill_block(const int fd, MetaData_t *toFill, const uint32_t count);
static void debug_log(const int opt_index, char **optarg, const int argc);
static int fill_superblock(SuperBlock_t *blockToFill, const int fd);
static SuperBlock_t *init_superblock_info();
static void writeCSV_GroupDescriptors();
static void writeCSV_superblock();
static void free_memory();

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
        fill_superblock(superblock_data, FD);
        writeCSV_superblock();

        /* Read into groupDescriptor table for each block group */
        uint32_t nGroupDescriptors = init_GroupDescriptorTable_info(group_descriptor_table);
        fill_GroupDescriptors(FD, group_descriptor_table, nGroupDescriptors);
        writeCSV_GroupDescriptors();

        for(i = 0; i < nGroupDescriptors; i++)
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
        if(!fill_block(fd, toFill->dataObjects, toFill->nDataObjects))
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
static int fill_block(const int fd, MetaData_t *toFill, const uint32_t count) {
        uint32_t i;
        int bytesRead = 0;
        for(i = 0; i < count; i++)
                bytesRead += pread(fd, &(toFill[i].value), toFill[i].size, toFill[i].offset);
        return bytesRead;
}

/* Reads the disk superblock, stores it in SuperBlock_t data structure,
 * and returns a pointer to the newly created data structure */
static SuperBlock_t *init_superblock_info() {

        /* allocate memory for SuperBlock_t data structure */
        SuperBlock_t *mSuperBlock = (SuperBlock_t *) malloc(sizeof(SuperBlock_t) + sizeof(MetaData_t) * DEFAULT_SUPERBLOCK_T.nDataObjects);
        if(mSuperBlock == NULL) {
                fprintf(stderr, "FATAL:: Unable to allocate memory for reading superblock\n");
                exit(1);
        }

        // Copy data from DEFAULT_SUPERBLOCK_T
        mSuperBlock->nDataObjects = DEFAULT_SUPERBLOCK_T.nDataObjects;
        memcpy(mSuperBlock, &DEFAULT_SUPERBLOCK_T,sizeof(SuperBlock_t) + (sizeof(MetaData_t) * DEFAULT_SUPERBLOCK_T.nDataObjects));

        return mSuperBlock;
}

static uint32_t init_GroupDescriptorTable_info(GroupDescriptor_t **groupDescriptorTable) {

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

/* Fills the given data structure based on the values it stores */
static int fill_GroupDescriptors(const int fd, GroupDescriptor_t **gdTable, const uint32_t nGDs) {

        uint32_t i, j;
        uint32_t blockCount         = superblock_data->dataObjects[2].value;
        uint32_t blocksPerGroup     = superblock_data->dataObjects[5].value;
        uint32_t numContainedBlocks = blocksPerGroup;
        for(i = 0; i < nGDs; i++) {
                uint32_t numObjects = gdTable[i]->nDataObjects;
                fprintf(stderr, "numObjects[%d] :: %d\n", i, numObjects);
        }
        /* read from disk */
        //fill_block(fd, gdTable[i]->dataObjects, gdTable[i]->nDataObjects);
        //for(j = 0; j < (); j++) {
        //        pread(fd, &(gdTable[i]->dataObjects[j].value), gdTable[i]->dataObjects[j].size, gdTable[i]->dataObjects[j].offset);
        //  fprintf(stderr, gdTable[i]->dataObjects[j].format, gdTable[i]->dataObjects[j].value);
        //fprintf(stderr, (j == (gdTable[i]->nDataObjects - 1)) ? "\n" : ",");
        //}
        /* get number of blocks contained within THIS group descriptor */
        // TODO :: Check if this logic is correct.
        // We are trying to see how many blocks are there in this group.
        // Check if this works when nGDs = 1;
        // if(i == (nGDs - 1))
        //         numContainedBlocks = blockCount - (blocksPerGroup * (i-1));
        // else
        //         numContainedBlocks = blocksPerGroup;
        //
        // gdTable[i]->dataObjects[0].value = numContainedBlocks;

        /* Print it out if VERBOSE */
        // if(VERBOSE)
        //         for(j = 0; j < (gdTable[i]->nDataObjects); j++) {
        //                 fprintf(stderr, gdTable[i]->dataObjects[j].format, gdTable[i]->dataObjects[j].value);
        //                 fprintf(stderr, (j == (gdTable[i]->nDataObjects - 1)) ? "\n" : ",");
        //         }
        if(VERBOSE)
                fprintf(stderr, "Inside fill_GD\n");
        return 1;
}

static void writeCSV_GroupDescriptors() {
        return;
}

/* if --VERBOSE is passed, logs to stdout */
static void debug_log(const int opt_index, char **optarg, const int argc) {
        fprintf(stderr,"--%s", long_options[opt_index].name);
        int i;
        for(i = 0; i < argc; i++)
                fprintf(stderr," %s", optarg[i]);
        fprintf(stderr,"\n");
}
