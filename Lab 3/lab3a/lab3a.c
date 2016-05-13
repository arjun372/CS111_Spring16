/**
    UCLA CS 111 - Spring '16
    Lab 3A - File System Dump
    Rahul Malavalli 204429252
    Arjun           504078752
 **/

#define FILE_MODE 0664

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>  /* for fprintf used in debug_log */
#include <stdint.h>
#include <getopt.h> /* Argument Options parse headers */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* option-specific variables */
static int VERBOSE = 0;
static struct option long_options[] = {
        {"verbose",          no_argument,  &VERBOSE,   1}
};

/* Info Structures */
typedef struct pos_info {
        char* name;
        char* format;
        int offset;
        int size;
} pos_info;

static pos_info superblock[] = {
        {"magic number", "%x", 56, 2},          // s_magic
        {"total number of inodes", "%d", 0, 4}, // s_inodes_count
        {"total number of blocks", "%d", 4, 4}, // s_blocks_count
        {"block size", "%d", 24, 4},            // s_log_block_size
        {"fragment size", "%d", 28, 4},         // s_log_frag_size
        {"blocks per group", "%d", 32, 4},      // s_blocks_per_group
        {"inodes per group", "%d", 40, 4},      // s_inodes_per_group
        {"fragments per group", "%d", 40, 4},   // s_frags_per_group
        {"first data block", "%d", 20, 4},      // s_first_data_block
};

static pos_info group_descriptor[] = {
        {"num contained blocks",    "%d", 1, 1},
        {"num free blocks",         "%d", 1, 1},
        {"num free inodes",         "%d", 1, 1},
        {"num directories",         "%d", 1, 1},
        {"free inode bitmap block", "%x", 14, 1}, // bg_free_inodes_count
        {"free block bitmap block", "%x", 12, 2}, // bg_free_blocks_count
        {"inode table(start)block", "%x", 1, 1}
};

static pos_info free_bitmap_entry[] = {
        {"map block number", "%x", 1, 1},
        {"entry number",     "%d", 1, 1}
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
        {"num blocks",        "%d", 1, 1},
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

/* Static function declarations */
static void debug_log(const int opt_index, char **optarg, const int argc);
static void read_options(int argc, char **argv);

int main (int argc, char **argv)
{
        /* Read --verbose option if it was passed */
        while(getopt_long_only(argc, argv, "", long_options, NULL) != 1)
                continue;

        if(!argc) {
                fprintf(stderr, "FATAL: no file passed as argument!\n");
                exit(1);
        }

        int FD;
        char *TargetFile  = argv[0];
        FD = open(TargetFile, O_RDONLY, FILE_MODE);
        if(FD < 0) {
                fprintf(stderr, "FATAL: unable to open file '%s'\n", TargetFile);
                exit(1);
        } else if (VERBOSE) fprintf(stderr, "Selecting file '%s'\n", TargetFile);

        exit(0);
}

/* if --VERBOSE is passed, logs to stdout */
static void debug_log(const int opt_index, char **optarg, const int argc) {
        fprintf(stderr,"--%s", long_options[opt_index].name);
        int i;
        for(i = 0; i < argc; i++)
                fprintf(stderr," %s", optarg[i]);
        fprintf(stderr,"\n");
}
