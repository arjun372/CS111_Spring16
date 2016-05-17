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
#define FILE_INODES                  "inode.csv"

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

static SuperBlock_t       *SUPERBLOCK_TABLE;
static GroupDescriptor_t **GROUP_DESCRIPTOR_TABLE;
static uint32_t NUM_GROUP_DESCRIPTORS;

/* option-specific variables */
static int VERBOSE = 0;
static struct option long_options[] = {
        {"verbose",          no_argument,  &VERBOSE,   1}
};

/* Static function declarations */
static void         debug_log(const int opt_index, char **optarg, const int argc);


static uint32_t     init_GROUP_DESCRIPTOR_TABLE_info();
static SuperBlock_t *init_superblock_info();

static int          fill_block(const int fd, MetaData_t *toFill, const uint32_t count);
static int          fill_superblock(const int fd);
static int          fill_GroupDescriptors(const int fd);

static void         writeCSV_GroupDescriptors();
static void         writeCSV_superblock();
static void         writeCSV_inode(const int fd);

static void         readAndWrite_freeBitmaps(const int fd);

static void         free_memory();
/* End static function declarations */


int main (int argc, char **argv)
{

        /* Getting options */
        uint32_t i, j;
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
        /* END Getting options */



        /* Process superblock information */
        SUPERBLOCK_TABLE = init_superblock_info();
        fill_superblock(FD);
        writeCSV_superblock();

        /* Read into groupDescriptor table for each block group */
        NUM_GROUP_DESCRIPTORS = init_GROUP_DESCRIPTOR_TABLE_info();
        fill_GroupDescriptors(FD);
        writeCSV_GroupDescriptors();

        readAndWrite_freeBitmaps(FD);

        writeCSV_inode(FD);

        /* Free memory associated with GROUP_DESCRIPTOR_TABLE */
        for(i = 0; i < NUM_GROUP_DESCRIPTORS; i++)
                free(GROUP_DESCRIPTOR_TABLE[i]);

        free(GROUP_DESCRIPTOR_TABLE);                // free gdTable

        /* Free memory associated with SUPERBLOCK_TABLE */
        free(SUPERBLOCK_TABLE);

        close(FD);                                 // close TargetFile
        exit(0);
}

static void readAndWrite_freeBitmaps(const int diskFD) {
        uint32_t i, j;
        uint32_t blockCount     = SUPERBLOCK_TABLE->dataObjects[2].value;
        uint32_t blocksPerGroup = SUPERBLOCK_TABLE->dataObjects[5].value;
        uint32_t nBlockGroups   = (blockCount + blocksPerGroup - 1) / blocksPerGroup;
        
        /* Stores a bitmap for each of the group descriptors */
        uint32_t *inodeBitmap[nBlockGroups];
        uint32_t *blockBitmap[nBlockGroups];
        
        unsigned int blockSize = SUPERBLOCK_TABLE->dataObjects[3].value;
        
        unsigned int blockBitmapStart, inodeBitmapStart;
        
        int fd = open(FILE_FREE_BITMAPS, CSV_WRITE_FLAGS, FILE_MODE);
        if(fd < 0) {
                fprintf(stderr, "FATAL(%d): %s\n", errno, strerror(errno));
                exit(1);
        } else if(VERBOSE) fprintf(stderr, "Writing Free Bitmaps: '%s'\n", FILE_SUPERBLOCK);
        
        /* Populate the bitmaps for each of the group descriptors */
        for (i = 0; i < nBlockGroups; ++i) {
                inodeBitmap[i] = malloc(SUPERBLOCK_TABLE->dataObjects[3].value);
                blockBitmap[i] = malloc(SUPERBLOCK_TABLE->dataObjects[3].value);
                inodeBitmapStart = GROUP_DESCRIPTOR_TABLE[i]->dataObjects[4].value;
                blockBitmapStart = GROUP_DESCRIPTOR_TABLE[i]->dataObjects[5].value;
                
                pread(  fd, 
                        inodeBitmap[i], 
                        blockSize,
                        inodeBitmapStart * blockSize);
                        
                pread(  fd, 
                        blockBitmap[i], 
                        blockSize, 
                        inodeBitmapStart * blockSize);
                        
                        
                uint32_t mask = 1;      // 000...001
                uint32_t *currimap = inodeBitmap[i], *currbmap = blockBitmap[i];
                mask = mask << 31;      // 100...000
                for (j = 0; j < blockSize; ++j) {
                        uint32_t ibit = 
                                ((currimap[j / 32] & mask) 
                                >> (31 - (j % 32)));
                        uint32_t bbit = 
                                ((currbmap[j / 32] & mask) 
                                >> (31 - (j % 32)));
                                
                        // Just testing
                        // TODO: simply add these things to the csv file in the appropriate order
                        dprintf(fd,
                                "Location: %d\nInode block: %d         ibit: %d\nBlock block: %d         bbit: %d\n\n",
                                j,
                                inodeBitmapStart,
                                ibit,
                                blockBitmapStart,
                                bbit);
                }       
        }
}

static void writeCSV_superblock() {
        int fd = open(FILE_SUPERBLOCK, CSV_WRITE_FLAGS, FILE_MODE);
        if(fd < 0) {
                fprintf(stderr, "FATAL(%d): %s\n", errno, strerror(errno));
                exit(1);
        } else if(VERBOSE) fprintf(stderr, "Writing CSV: '%s'\n", FILE_SUPERBLOCK);

        uint32_t i;
        for(i = 0; i < (SUPERBLOCK_TABLE->nDataObjects); i++) {
                dprintf(fd, SUPERBLOCK_TABLE->dataObjects[i].format, SUPERBLOCK_TABLE->dataObjects[i].value);
                dprintf(fd, (i == (SUPERBLOCK_TABLE->nDataObjects - 1)) ? "\n" : ",");
        }
        close(fd);
}

/* Fills the given data structure based on the values it stores */
static int fill_superblock(const int fd) {
        /* If filling the block from disk failed, then return 0 */
        if(!fill_block(fd, SUPERBLOCK_TABLE->dataObjects, SUPERBLOCK_TABLE->nDataObjects))
                return 0;

        /** Now do SuperBlock_t specific actions */
        uint32_t correct_BlockSize   = 1024 << SUPERBLOCK_TABLE->dataObjects[3].value;
        uint32_t correct_FragSize    = (SUPERBLOCK_TABLE->dataObjects[4].value > 0) ? (1024 << SUPERBLOCK_TABLE->dataObjects[4].value) : (1024 >> -(SUPERBLOCK_TABLE->dataObjects[4].value));
        SUPERBLOCK_TABLE->dataObjects[3].value = correct_BlockSize;
        SUPERBLOCK_TABLE->dataObjects[4].value = correct_FragSize;

        /* Print it out if VERBOSE */
        if(VERBOSE) {
                uint32_t i;
                for(i = 0; i < (SUPERBLOCK_TABLE->nDataObjects); i++) {
                        fprintf(stderr, SUPERBLOCK_TABLE->dataObjects[i].format, SUPERBLOCK_TABLE->dataObjects[i].value);
                        fprintf(stderr, (i == (SUPERBLOCK_TABLE->nDataObjects - 1)) ? "\n" : ",");
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

static uint32_t init_GROUP_DESCRIPTOR_TABLE_info() {
        uint32_t i, j;
        uint32_t blockCount     = SUPERBLOCK_TABLE->dataObjects[2].value;
        uint32_t blocksPerGroup = SUPERBLOCK_TABLE->dataObjects[5].value;
        uint32_t nBlockGroups   = (blockCount + blocksPerGroup - 1) / blocksPerGroup;

        /* allocate memory for GroupDescriptor_Table members */
        GROUP_DESCRIPTOR_TABLE = (GroupDescriptor_t **) malloc(sizeof(GroupDescriptor_t *) * nBlockGroups);
        if(GROUP_DESCRIPTOR_TABLE == NULL) goto MEM_ERR;

        /* The block group descriptor table starts on the first block following the superblock.
         * This would be the third block on a 1KiB block file system, or the second block for
         * 2KiB and larger block file systems. Shadow copies of the block	group descriptor table
         * are also stored with every copy of the superblock. */
        uint32_t blockSize           = SUPERBLOCK_TABLE->dataObjects[3].value;
        uint32_t gdTable_blockOffset = (blockSize <= 1024) ? 2 : 1;
        uint32_t startOffset         = blockSize * gdTable_blockOffset;

        uint32_t bgd_size            = 32;
        uint32_t bgdTable_size       = bgd_size * nBlockGroups;

        /* Depending on how many block groups are defined, this table can require multiple blocks
         * of storage. Always refer to the superblock in case of doubt. */

        /* instantiate each group_descriptor */
        for(i = 0; i < nBlockGroups; i++) {
                /* allocate memory for SuperBlock_t data structure */
                GROUP_DESCRIPTOR_TABLE[i] = (GroupDescriptor_t *) malloc(sizeof(GroupDescriptor_t)
                                                                         + sizeof(MetaData_t) * DEFAULT_GROUP_DESCR_T.nDataObjects);
                if(GROUP_DESCRIPTOR_TABLE[i] == NULL) {
                        fprintf(stderr, "FATAL:: Unable to allocate memory for reading group descriptor\n");
                        exit(1);
                }

                // Copy data from DEFAULT_GROUP_DESCR_T
                GROUP_DESCRIPTOR_TABLE[i]->nDataObjects = DEFAULT_GROUP_DESCR_T.nDataObjects;
                memcpy(GROUP_DESCRIPTOR_TABLE[i], &DEFAULT_GROUP_DESCR_T,
                       sizeof(GroupDescriptor_t)
                       + (sizeof(MetaData_t) * DEFAULT_GROUP_DESCR_T.nDataObjects));

                uint32_t offset;
                for (j = 0; j < GROUP_DESCRIPTOR_TABLE[i]->nDataObjects; j++) {
                        offset = startOffset + (i * bgd_size);
                        GROUP_DESCRIPTOR_TABLE[i]->dataObjects[j].offset =
                                offset + GROUP_DESCRIPTOR_TABLE[i]->dataObjects[j].offset;

                        if (VERBOSE) {
                                printf("offset: %d\nsize: %d\nformat: %s\n\n",
                                       GROUP_DESCRIPTOR_TABLE[i]->dataObjects[j].offset,
                                       GROUP_DESCRIPTOR_TABLE[i]->dataObjects[j].size,
                                       GROUP_DESCRIPTOR_TABLE[i]->dataObjects[j].format);
                        }
                }
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
static int fill_GroupDescriptors(const int fd) {

        uint32_t i, j;
        uint32_t blockCount         = SUPERBLOCK_TABLE->dataObjects[2].value;
        uint32_t blocksPerGroup     = SUPERBLOCK_TABLE->dataObjects[5].value;
        uint32_t numContainedBlocks = blocksPerGroup;
        for(i = 0; i < NUM_GROUP_DESCRIPTORS; i++) {

                /* read from disk */
                fill_block(fd, GROUP_DESCRIPTOR_TABLE[i]->dataObjects, GROUP_DESCRIPTOR_TABLE[i]->nDataObjects);

                /* get number of blocks contained within THIS group descriptor */
                GROUP_DESCRIPTOR_TABLE[i]->dataObjects[0].value = (i == (NUM_GROUP_DESCRIPTORS - 1)) ? (blockCount % blocksPerGroup) : blocksPerGroup;

                /* Print it out if VERBOSE */
                if(VERBOSE)
                        for(j = 0; j < (GROUP_DESCRIPTOR_TABLE[i]->nDataObjects); j++) {
                                fprintf(stderr, GROUP_DESCRIPTOR_TABLE[i]->dataObjects[j].format, GROUP_DESCRIPTOR_TABLE[i]->dataObjects[j].value);
                                fprintf(stderr, (j == (GROUP_DESCRIPTOR_TABLE[i]->nDataObjects - 1)) ? "\n" : ",");
                        }
        }
        return 1;
}

static void writeCSV_GroupDescriptors() {
        int fd = open(FILE_GROUP_DESCRIPTOR, CSV_WRITE_FLAGS, FILE_MODE);
        if(fd < 0) {
                fprintf(stderr, "FATAL(%d): %s\n", errno, strerror(errno));
                exit(1);
        } else if(VERBOSE) fprintf(stderr, "Writing CSV: '%s'\n", FILE_GROUP_DESCRIPTOR);

        uint32_t i,j;
        for(i = 0; i < NUM_GROUP_DESCRIPTORS; i++) {
                for(j = 0; j < (GROUP_DESCRIPTOR_TABLE[i]->nDataObjects); j++) {
                        dprintf(fd, GROUP_DESCRIPTOR_TABLE[i]->dataObjects[j].format, GROUP_DESCRIPTOR_TABLE[i]->dataObjects[j].value);
                        dprintf(fd, (j == (GROUP_DESCRIPTOR_TABLE[i]->nDataObjects - 1)) ? "\n" : ",");
                }
        }
        close(fd);
        return;
}

static void writeCSV_inode(const int FD) {

        int fd = open(FILE_INODES, CSV_WRITE_FLAGS, FILE_MODE);
        if(fd < 0) {
                fprintf(stderr, "FATAL(%d): %s\n", errno, strerror(errno));
                exit(1);
        } else if(VERBOSE) fprintf(stderr, "Writing CSV: '%s'\n", FILE_INODES);

        uint16_t data0;
        uint32_t i, j, k, data;
        uint32_t blockSize          = SUPERBLOCK_TABLE->dataObjects[3].value;
        uint32_t inodeCount         = SUPERBLOCK_TABLE->dataObjects[1].value;
        uint32_t numInodesPerGroup  = SUPERBLOCK_TABLE->dataObjects[6].value;
        uint32_t numInodesLastGroup = inodeCount % numInodesPerGroup;
        uint32_t inodeSize          = 128;

        /* run this for each group descriptor */
        for(i = 0; i < NUM_GROUP_DESCRIPTORS; i++)
        {
                uint32_t TBL_BLK_OFF = GROUP_DESCRIPTOR_TABLE[i]->dataObjects[6].value;
                uint32_t numInodes = (i == (NUM_GROUP_DESCRIPTORS - 1)) ? numInodesLastGroup : numInodesPerGroup;
                if(VERBOSE) fprintf(stderr, "Processing descriptor (%d)..with inodes : %d\n", i, numInodes);
                for(j = 0; j < numInodes; j++) {

                        uint32_t iNODE_OFF = (inodeSize * j) + (TBL_BLK_OFF * blockSize);

                        dprintf(fd, "%d,", j);
                        //TODO :: Read inode number!! :   pread(FD, &data, sizeof(data), )

                        /* read file-mode */
                        pread(FD, &data0, sizeof(data0), iNODE_OFF + 0);
                        if     (data0 & 0xA000) dprintf(fd, "s,");
                        else if(data0 & 0x4000) dprintf(fd, "d,");
                        else if(data0 & 0x8000) dprintf(fd, "f,");
                        else                    dprintf(fd, "?,");

                        // TODO : FILE_MODE
                        dprintf(fd, "mode,");

                        // TODO : FILE_OWNER
                        dprintf(fd, "owner,");

                        // TODO : FILE_GROUP
                        dprintf(fd, "group,");

                        /* read LINK_COUNT */
                        pread(FD, &data0, sizeof(data0), iNODE_OFF + 26);
                        dprintf(fd, "%d,", data0);

                        /* read CREATION_TIME */
                        pread(FD, &data, sizeof(data), iNODE_OFF + 12);
                        dprintf(fd, "%x,", data);

                        /* read MODIFIED_TIME */
                        pread(FD, &data, sizeof(data), iNODE_OFF + 16);
                        dprintf(fd, "%x,", data);

                        /* read ACCESS_TIME */
                        pread(FD, &data, sizeof(data), iNODE_OFF +  8);
                        dprintf(fd, "%x,", data);

                        /* read FILE_SIZE */
                        pread(FD, &data, sizeof(data), iNODE_OFF +  4);
                        dprintf(fd, "%d,", data);

                        /* read NUMBER_OF_BLOCKS */
                        pread(FD, &data, sizeof(data), iNODE_OFF + 28);
                        dprintf(fd, "%d,", data);

                        /* read BLOCKS+POINTERS */
                        for(k = 0; k < 15; k++) {
                                pread(FD, &data, sizeof(data), iNODE_OFF + 40 + (4*k));
                                dprintf(fd, "%d", data);
                                dprintf(fd, (k==14) ? "\n" : ",");
                        }
                }
        }
        close(fd);
}

/* if --VERBOSE is passed, logs to stdout */
static void debug_log(const int opt_index, char **optarg, const int argc) {
        fprintf(stderr,"--%s", long_options[opt_index].name);
        int i;
        for(i = 0; i < argc; i++)
                fprintf(stderr," %s", optarg[i]);
        fprintf(stderr,"\n");
}
