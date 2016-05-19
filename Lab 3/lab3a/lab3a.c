/**
    UCLA CS 111 - Spring '16
    Lab 3A - File System Dump
    Rahul Malavalli - 204429252
    Arjun Arjun     - 504078752
 **/

#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <getopt.h> /* Argument Options parse headers */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>  /* for fprintf used in debug_log */
#include <inttypes.h>
#include "lab3a.h"

static SuperBlock_t      *SUPERBLOCK_TABLE;
static GroupDescriptor_t **GROUP_DESCRIPTOR_TABLE;
static uint32_t NUM_GROUP_DESCRIPTORS;
static uint8_t           *BITMAP_BLOCKS;
static uint8_t           *BITMAP_INODES;

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

        free_memory();
        close(FD);     // close TargetFile
        exit(0);
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


static int dir_doWrite(int readfd, int writefd,  uint32_t parentInode, uint32_t block, uint32_t currCount){
        if (block == 0) return currCount;
        uint32_t blockSize = (SUPERBLOCK_TABLE->dataObjects[3].value);
        //uint32_t *entry = calloc(blockSize/sizeof(uint32_t), sizeof(uint32_t));
        uint32_t entry[blockSize];
        //entry = memset(entry, 0, blockSize/sizeof(uint32_t));
        uint32_t pos = 0;        // byte offset at beginning of current directory block
        uint32_t count = currCount;
        pread(readfd, entry, blockSize, block * blockSize);
        uint16_t rec_len;
        // Only continue while loop if there is enough space for an entry to exist,
        //      AND if the inode of the current entry is valid non-zero)
        while(((pos + 9) < blockSize)) {
                if (VERBOSE) printf("In block %x for pos %d\n", block, pos);
                // name = (char*) &entry[pos + 2];
                rec_len = entry[pos+1] << 16;
                if (!rec_len) break;
                dprintf(writefd, "%d,%d,%d,%d,%d,%s\n",
                        parentInode,                            // parent inode
                        count++,                                // entry count
                        (uint16_t) (rec_len),                   // entry length
                        (char) (entry[pos + 1] << 8),           // name length
                        (char) entry[pos + 1],                  // inode number of file
                        "name please");                         // name

                pos += ((uint16_t) (rec_len));
        }
        return count;
}

static int dir_doWrite2(int readfd, int writefd,  uint32_t parentInode, uint32_t blockNum, uint32_t currCount){

        uint32_t blockSize       = (SUPERBLOCK_TABLE->dataObjects[3].value);
        uint32_t blockByteOffset = blockNum * blockSize;
        uint32_t prevEntryLength = 0;
        int count                = 0;

        uint32_t *inode_num  = (uint32_t *) malloc(sizeof(uint32_t));       // 4 bytes
        uint16_t *rec_len    = (uint16_t *) malloc(sizeof(uint16_t));       // 2 bytes
        uint8_t  *name_len   = (uint8_t *)  malloc(sizeof(uint8_t));        // 1 byte
        char     *name       = (char *)     malloc(sizeof(char) * 255);  // 0 to 255 bytes

        while(1) {
                pread(readfd, inode_num,         4, blockByteOffset + prevEntryLength + 0);
                pread(readfd, rec_len,           2, blockByteOffset + prevEntryLength + 4);
                pread(readfd, name_len,          1, blockByteOffset + prevEntryLength + 6);
                pread(readfd, name,      *name_len, blockByteOffset + prevEntryLength + 8); // read upto @param name_len bytes into name

                prevEntryLength += rec_len;
                if (VERBOSE) printf("In block %x, count: %d\n", blockNum, count);
                dprintf(writefd, "%d,%d,%d,%d,%d,%s\n",
                        parentInode,                     // parent inode
                        count++,                         // entry count
                        rec_len,                         // entry length
                        name_len,                        // name length
                        inode_num,                       // inode number of file
                        name);                           // name

                if(*name_len + 8 == rec_len)
                        break;
        }
        free(inode_num);
        free(rec_len);
        free(name_len);
        free(name);
        return count;
}

static void writeCSV_dir(int readfd, int writefd, uint32_t parentInode, uint32_t blocks[15]) {
        uint32_t i, j, k, count = 0;
        if (VERBOSE)
                for(i = 0; i < 15; ++i) {
                        fprintf(stdout, "%x\n", blocks[i]);
                }
        uint32_t blockSize = SUPERBLOCK_TABLE->dataObjects[3].value;
        uint32_t entry[blockSize];
        uint32_t zero[blockSize];
        for(i = 0; i < (blockSize/4); ++i) zero[i] = 0;
        uint32_t block;
        // char* name;
        for(i = 0; i < 12; ++i) {
                block = blocks[i];
                if (block == 0) return;   // This is last bock, return count
                count = dir_doWrite2(readfd, writefd, parentInode, blocks[i], count);
        }


        uint32_t numPtrsPerBlock = blockSize/4;
        // Single Indirect block Pointer
        block = blocks[12];
        if (block == 0) return;    // This is the ending block, end and return count
        pread(readfd, entry, blockSize, block * blockSize);
        for (i = 0; i < numPtrsPerBlock; ++i) {
                if (entry[i] == 0) return;
                count = dir_doWrite(readfd, writefd, parentInode, entry[i], count);
        }

        // Double Indirect Block Pointer
        block = blocks[13];
        uint32_t ind1[blockSize];
        pread(readfd, entry, blockSize, block * blockSize);
        for (i = 0; i < numPtrsPerBlock; ++i) {
                if(entry[i] == 0) return;
                pread(readfd, ind1, blockSize, entry[i] * blockSize);
                for (j = 0; j < numPtrsPerBlock; ++j) {
                        if (ind1[j] == 0) return;
                        count = dir_doWrite(readfd, writefd, parentInode, ind1[j], count);
                }
        }

        // Triple Indirect Block Pointer
        block = blocks[14];
        uint32_t ind2[blockSize];
        pread(readfd, entry, blockSize, block * blockSize);
        for (i = 0; i < numPtrsPerBlock; ++i) {
                if(entry[i] == 0) return;
                pread(readfd, ind1, blockSize, entry[i] * blockSize);
                for (j = 0; j < numPtrsPerBlock; ++j) {
                        if (ind1[j] == 0) return;
                        pread(readfd, ind2, blockSize, ind1[j] * blockSize);
                        for (k = 0; k < numPtrsPerBlock; ++k) {
                                if (ind2[k] == 0) return;
                                count = dir_doWrite(readfd, writefd, parentInode, ind2[k], count);
                        }
                }
        }
}

// static void writeCSV_indirectBlocks(int writefd, uint32_t iblocks[3]) {
//
// }

static void writeCSV_inode(const int FD) {

        int fd = open(FILE_INODES, CSV_WRITE_FLAGS, FILE_MODE);
        if(fd < 0) {
                fprintf(stderr, "FATAL(%d): %s\n", errno, strerror(errno));
                exit(1);
        } else if(VERBOSE) fprintf(stderr, "Writing CSV: '%s'\n", FILE_INODES);

        int dirfd = open(FILE_DIRECTORY_ENTRIES, CSV_WRITE_FLAGS, FILE_MODE);
        if(dirfd < 0) {
                fprintf(stderr, "FATAL(%d): %s\n", errno, strerror(errno));
                exit(1);
        } else if(VERBOSE) fprintf(stderr, "Writing Directory entries: '%s'\n", FILE_DIRECTORY_ENTRIES);

        uint16_t data0, file_type;
        uint32_t i, j, k, l=0, data;
        uint32_t blockSize          = SUPERBLOCK_TABLE->dataObjects[3].value;
        uint32_t inodeCount         = SUPERBLOCK_TABLE->dataObjects[1].value;
        uint32_t numInodesPerGroup  = SUPERBLOCK_TABLE->dataObjects[6].value;
        uint32_t numInodesLastGroup = inodeCount % numInodesPerGroup;
        uint32_t inodeSize          = 128;
        uint32_t ext2BlockSize      = 512;

        uint32_t dirBlocksArray[15];
        /* run this for each group descriptor */
        for(i = 0; i < NUM_GROUP_DESCRIPTORS; i++)
        {
                uint32_t TBL_BLK_OFF = GROUP_DESCRIPTOR_TABLE[i]->dataObjects[6].value;
                // uint32_t numInodes = (i == (NUM_GROUP_DESCRIPTORS - 1)) ? numInodesLastGroup : numInodesPerGroup;
                uint32_t numInodes = SUPERBLOCK_TABLE->dataObjects[6].value;

                if(VERBOSE) fprintf(stderr, "Processing descriptor (%d)..with inodes : %d\n", i, numInodes);

                for(j = 0; j < numInodes; j++) {

                        if (BITMAP_INODES[l++] == 0) continue;

                        uint32_t iNODE_OFF   = (inodeSize * j) + (TBL_BLK_OFF * blockSize);
                        // uint32_t inodeNumber = (j + 1) + (numInodesPerGroup * i);
                        uint32_t inodeNumber = l;

                        /* iNode Number */
                        dprintf(fd, "%d,", inodeNumber);

                        /* read file-type */
                        pread(FD, &file_type, sizeof(file_type), iNODE_OFF + 0);

                        if     ((file_type & 0xA000) == 0xA000) dprintf(fd, "s,");
                        else if((file_type & 0x8000) == 0x8000) dprintf(fd, "f,");
                        else if((file_type & 0x4000) == 0x4000) dprintf(fd, "d,");
                        else                                dprintf(fd, "?,");

                        // TODO : FILE_MODE
                        dprintf(fd, "%o,", file_type);

                        // TODO : FILE_OWNER
                        pread(FD, &data0, sizeof(data0), iNODE_OFF +  2);
                        dprintf(fd, "%d,", data0);

                        // TODO : FILE_GROUP
                        pread(FD, &data0, sizeof(data0), iNODE_OFF + 24);
                        dprintf(fd, "%d,", data0);

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
                        data = data * ext2BlockSize / blockSize;
                        dprintf(fd, "%d,", data);

                        /* read BLOCKS+POINTERS */
                        for(k = 0; k < 15; k++) {
                                pread(FD, &data, sizeof(data), iNODE_OFF + 40 + (4*k));
                                dprintf(fd, "%x", data);
                                dprintf(fd, (k==14) ? "\n" : ",");

                                dirBlocksArray[k] = data;
                        }

                        // Write Directory Entry Structure
                        if((file_type & 0x4000) == 0x4000) {
                                writeCSV_dir(FD, dirfd, inodeNumber, dirBlocksArray);
                        }

                        // Write indirect blocks, if any
                        // writeCSV_indirectBlocks();
                }
        }
        close(fd);
}

static void free_memory() {
        uint32_t i;
        for(i = 0; i < NUM_GROUP_DESCRIPTORS; i++)
                free(GROUP_DESCRIPTOR_TABLE[i]);

        free(GROUP_DESCRIPTOR_TABLE);
        free(SUPERBLOCK_TABLE);
        free(BITMAP_BLOCKS);
        free(BITMAP_INODES);
}

/* if --VERBOSE is passed, logs to stdout */
static void debug_log(const int opt_index, char **optarg, const int argc) {
        fprintf(stderr,"--%s", long_options[opt_index].name);
        int i;
        for(i = 0; i < argc; i++)
                fprintf(stderr," %s", optarg[i]);
        fprintf(stderr,"\n");
}

static int isFree(void *buffer, uint32_t pos) {
        uint32_t nbyte = pos/8;
        int nbit       = pos % 8;
        char *ptr      = buffer + nbyte;
        return !((*ptr)&(1 << nbit));
}

static void readAndWrite_freeBitmaps(const int diskFD) {

        uint32_t i, j, iBMP_OFFSET, bBMP_OFFSET;
        uint32_t blockCount     = SUPERBLOCK_TABLE->dataObjects[2].value;
        uint32_t inodeCount     = SUPERBLOCK_TABLE->dataObjects[1].value;
        uint32_t blockSize      = SUPERBLOCK_TABLE->dataObjects[3].value;
        uint32_t blocksPerGroup = SUPERBLOCK_TABLE->dataObjects[5].value;
        uint32_t inodesPerGroup = SUPERBLOCK_TABLE->dataObjects[6].value;
        uint32_t nBlockGroups   = (blockCount + blocksPerGroup - 1) / blocksPerGroup;
        uint32_t bitsInBMP      = blockSize * 8;
        uint8_t BYTE_MASK       = 0x80;  /* 1000 0000 */
        uint8_t iBIT            = 0x00;
        uint8_t bBIT            = 0x00;

        /* Stores a bitmap for each of the group descriptors */
        BITMAP_INODES      = (uint8_t*) malloc(inodeCount * sizeof(uint8_t));
        BITMAP_BLOCKS      = (uint8_t*) malloc(blockCount * sizeof(uint8_t));
        uint8_t *currI_BMP = (uint8_t*) malloc(blockSize);
        uint8_t *currB_BMP = (uint8_t*) malloc(blockSize);
        if(BITMAP_INODES == NULL || BITMAP_INODES == NULL || currB_BMP == NULL || currI_BMP == NULL) {
                fprintf(stderr, "FATAL:: Memory error. bye bye! \n");
                exit(1);
        }
        int fd = open(FILE_FREE_BITMAPS, CSV_WRITE_FLAGS, FILE_MODE);
        if(fd < 0) {
                fprintf(stderr, "FATAL(%d): %s\n", errno, strerror(errno));
                exit(1);
        } else if(VERBOSE) fprintf(stderr, "Writing Free Bitmaps: '%s'\n", FILE_FREE_BITMAPS);


        uint32_t maxInodes = 0, maxBlocks = 0;
        uint32_t inodeindex = 1, blockindex = 1;

        /* Populate the bitmaps for each of the group descriptors */
        for (i = 0; i < nBlockGroups; i++) {

                iBMP_OFFSET = GROUP_DESCRIPTOR_TABLE[i]->dataObjects[4].value;
                bBMP_OFFSET = GROUP_DESCRIPTOR_TABLE[i]->dataObjects[5].value;

                currI_BMP = memset(currI_BMP, 0, blockSize);
                currB_BMP = memset(currB_BMP, 0, blockSize);

                pread(diskFD, currI_BMP, blockSize, iBMP_OFFSET * blockSize);
                pread(diskFD, currB_BMP, blockSize, bBMP_OFFSET * blockSize);

                if (i < nBlockGroups - 1) {
                        maxInodes += inodesPerGroup;
                        maxBlocks += blocksPerGroup;
                } else {
                        maxInodes = inodeCount;
                        maxBlocks = blockCount;
                }

                /* Now check if each bit in @param blockSize array is 1 or 0 */
                // 8192
                BYTE_MASK = 0x01;
                for (j = 0; j < bitsInBMP; j++) {

                        if(inodeindex <= maxInodes) {
                                iBIT = !!(currI_BMP[j/8] & BYTE_MASK); //1024
                                if (!iBIT) dprintf(fd, "%x,%" PRIu32 "\n", iBMP_OFFSET, inodeindex);
                                BITMAP_INODES[inodeindex-1] = (uint8_t) !!iBIT;
                                inodeindex++;
                        }

                        if(blockindex <= maxBlocks) {
                                bBIT = !!(currB_BMP[j/8] & BYTE_MASK);
                                if (!bBIT) dprintf(fd, "%x,%" PRIu32 "\n", bBMP_OFFSET, blockindex);
                                BITMAP_BLOCKS[blockindex-1] = (uint8_t) !!bBIT;
                                blockindex++;
                        }

                        if(VERBOSE) fprintf(stderr, "mask[%d] :: %x\n", i, BYTE_MASK);

                        BYTE_MASK = (BYTE_MASK == 0x80) ? 0x01 : (BYTE_MASK << 1);
                }
        }

        printf("here\n");
        free(currI_BMP);
        free(currB_BMP);
        printf("there\n");
}
// static void readAndWrite_freeBitmaps(const int diskFD) {
//
//
//         uint32_t I_POS = 0;
//         uint32_t B_POS = 0;                    //20k
//         uint32_t i, j, iBMP_OFFSET, bBMP_OFFSET;
//         uint32_t inodeCount     = SUPERBLOCK_TABLE->dataObjects[1].value;
//         uint32_t blockCount     = SUPERBLOCK_TABLE->dataObjects[2].value;
//         uint32_t blockSize      = SUPERBLOCK_TABLE->dataObjects[3].value;
//         uint32_t blocksPerGroup = SUPERBLOCK_TABLE->dataObjects[5].value;
//         uint32_t inodesPerGroup = SUPERBLOCK_TABLE->dataObjects[6].value;
//         uint32_t nBlockGroups   = (blockCount + blocksPerGroup - 1) / blocksPerGroup;
//         uint32_t bitsInBMP      = blockSize * 8;
//         uint8_t BYTE_MASK       = 0x80;  /* 1000 0000 */
//
//         int fd = open(FILE_FREE_BITMAPS, CSV_WRITE_FLAGS, FILE_MODE);
//         if(fd < 0) {fprintf(stderr, "FATAL(%d): %s\n", errno, strerror(errno)); exit(1); }
//         else if(VERBOSE) fprintf(stderr, "Writing Free Bitmaps: '%s'\n", FILE_FREE_BITMAPS);
//
//         /* Stores a bitmap for each of the group descriptors */
//         BITMAP_INODES               = (uint32_t*) malloc(inodeCount);
//         BITMAP_BLOCKS               = (uint32_t*) malloc(blockCount);
//         uint8_t* current_iNode_BMP = (uint8_t*)  malloc(blockSize);
//         uint8_t* current_Block_BMP = (uint8_t*)  malloc(blockSize);
//         if(BITMAP_INODES == NULL || BITMAP_BLOCKS == NULL || current_iNode_BMP == NULL || current_Block_BMP == NULL) {
//                 fprintf(stderr, "FATAL:: Memory error. bye bye! \n");
//                 exit(1);
//         }
//
//         /* Populate the bitmaps for each of the group descriptors */
//         for (i = 0; i < nBlockGroups; i++) {
//
//                 iBMP_OFFSET = GROUP_DESCRIPTOR_TABLE[i]->dataObjects[4].value;
//                 bBMP_OFFSET = GROUP_DESCRIPTOR_TABLE[i]->dataObjects[5].value;
//
//                 //current_iNode_BMP = memset(current_iNode_BMP, 0, blockSize);
//                 //current_Block_BMP = memset(current_Block_BMP, 0, blockSize);
//
//                 pread(diskFD, current_iNode_BMP, blockSize, iBMP_OFFSET * blockSize);
//                 pread(diskFD, current_Block_BMP, blockSize, bBMP_OFFSET * blockSize);
//
//                 /* Now check if each bit in @param blockSize array is 1 or 0 */
//                 BYTE_MASK = 0x80;
//                 for (j = 0; j < bitsInBMP; j++) { //8192
//
//                         if(VERBOSE) fprintf(stderr, "I_POS : %d, B_BOS: %d\n", I_POS, B_POS);
//
//                         //isFree(current_iNode_BMP, j);
//                         //isFree(current_Block_BMP, j);
//
//                         /* Set all bitMasks to NULL */
//                         if(j < inodesPerGroup && I_POS < inodeCount) {
//                                 BITMAP_INODES[I_POS] = !!(current_iNode_BMP[j/8] & BYTE_MASK);
//                                 if(!BITMAP_INODES[I_POS])
//                                         dprintf(fd, "%x,%d\n", iBMP_OFFSET, j + 0 + (i * inodesPerGroup));
//                                 I_POS++;
//                         }
//
//
//
//                         if(j < blocksPerGroup && B_POS < blockCount) {
//                                 BITMAP_BLOCKS[B_POS] = !!(current_Block_BMP[j/8] & BYTE_MASK);
//                                 if(!BITMAP_BLOCKS[B_POS])
//                                         dprintf(fd, "%x,%d\n", bBMP_OFFSET, j + 1 + (i * blocksPerGroup));
//                                 B_POS++;
//                         }
//
//                         //if(VERBOSE) fprintf(stderr, "mask[%d] :: %x\n", i, BYTE_MASK);
//
//                         BYTE_MASK = (BYTE_MASK == 0x01) ? 0x80 : (BYTE_MASK >> 1);
//                 }
//         }
//
//         printf("here\n");
//         free(current_iNode_BMP);
//         free(current_Block_BMP);
//         printf("there\n");
// }
