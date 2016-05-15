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

#include "lab3a.h"

/* global diskorg_t variables */
/** Single pointer if there is only ONE data structure (like superblock)**/
/** Double pointer if there is an array of data structures **/
static SuperBlock_t *superblock_data;
static GroupDescriptor_t **group_descriptors;

/* option-specific variables */
static int VERBOSE = 0;
static struct option long_options[] = {
        {"verbose",          no_argument,  &VERBOSE,   1}
};

/* Static function declarations */
static void debug_log(const int opt_index, char **optarg, const int argc);
static int fill_block(Block_t *blockToFill, int fd);
static SuperBlock_t *init_superblock_info();

int main (int argc, char **argv)
{
        /* Read --verbose option if it was passed */
        //  while(getopt_long_only(argc, argv, "", long_options, NULL) != 1)
        //            continue;

        if(argc <= 1) {
                fprintf(stderr, "FATAL: no file passed as argument!\n");
                exit(1);
        }

        int FD;
        char *TargetFile  = argv[1];
        FD = open(TargetFile, O_RDONLY, FILE_MODE);
        if(FD < 0) {
                fprintf(stderr, "FATAL: unable to open file '%s'\n", TargetFile);
                exit(1);
        } else fprintf(stderr, "Selecting file '%s'\n", TargetFile);

        superblock_data = init_superblock_info();
        if(fill_block(superblock_data, FD))
                fprintf(stderr, "Get SUPERBLOCK information :: SUCCESS\n");
        else
                fprintf(stderr, "Get SUPERBLOCK information :: FAILURE\n");


        free(superblock_data);
        exit(0);
}

/* Fills the given data structure based on the values it stores */
static int fill_block(Block_t *toFill, int fd) {
        uint32_t i;
        int bytesRead = 1;
        for(i = 0; i < (toFill->nDataObjects); i++) {
                bytesRead &= pread(fd, &(toFill->dataObjects[i].value), toFill->dataObjects[i].size, toFill->dataObjects[i].offset);
                fprintf(stderr, "VAL:: %d\n", toFill->dataObjects[i].value);
        }
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
        MetaData_t *mDataObjects = (MetaData_t *) malloc(sizeof(MetaData_t) * SUPERBLOCK_OBJECTS);
        if(mDataObjects == NULL) {
                fprintf(stderr, "FATAL:: Unable to allocate memory for reading superblock\n");
                free(mSuperBlock);
                exit(1);
        }

        /* assign this new data structure to superblock */
        mSuperBlock->nDataObjects = SUPERBLOCK_OBJECTS;
        mSuperBlock->dataObjects  = mDataObjects;

        struct metadata magicNumber    = {SUPERBLOCK_OFF + 56, 0, 2, "%x"}; // s_magic
        struct metadata inodeCount     = {SUPERBLOCK_OFF + 0,  0, 4, "%d"}; // s_inodes_count
        struct metadata blockCount     = {SUPERBLOCK_OFF + 4,  0, 4, "%d"}; // s_blocks_count
        struct metadata blockSize      = {SUPERBLOCK_OFF + 24, 0, 4, "%d"}; // s_log_block_size
        struct metadata fragSize       = {SUPERBLOCK_OFF + 28, 0, 4, "%d"}; // s_log_frag_size
        struct metadata blocksPerGroup = {SUPERBLOCK_OFF + 32, 0, 4, "%d"}; // s_blocks_per_group
        struct metadata inodesPerGroup = {SUPERBLOCK_OFF + 40, 0, 4, "%d"}; // s_inodes_per_group
        struct metadata fragsPerGroup  = {SUPERBLOCK_OFF + 36, 0, 4, "%d"}; // s_frags_per_group
        struct metadata firstDataBlock = {SUPERBLOCK_OFF + 20, 0, 4, "%d"}; // s_first_data_block

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

/* if --VERBOSE is passed, logs to stdout */
static void debug_log(const int opt_index, char **optarg, const int argc) {
        fprintf(stderr,"--%s", long_options[opt_index].name);
        int i;
        for(i = 0; i < argc; i++)
                fprintf(stderr," %s", optarg[i]);
        fprintf(stderr,"\n");
}
