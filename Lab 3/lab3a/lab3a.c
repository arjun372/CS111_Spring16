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
typedef struct pos_info{
  char* name;
  char* format;
  int offset;
  int size;
} pos_info;
static pos_info superblock[] = {
  {"magic number", "%x", 56, 2},                // s_magic
  {"total number of inodes", "%d", 0, 4},       // s_inodes_count
  {"total number of blocks", "%d", 4, 4},       // s_blocks_count
  {"block size", "%d", 24, 4},                  // s_log_block_size
  {"fragment size", "%d", 28, 4},               // s_log_frag_size
  {"blocks per group", "%d", 32, 4},            // s_blocks_per_group
  {"inodes per group", "%d", 40, 4},            // s_inodes_per_group
  {"fragments per group", "%d", 40, 4},         // s_frags_per_group
  {"first data block", "%d", 20, 4},            // s_first_data_block
};

/* Static function declarations */
static void debug_log(const int opt_index, char **optarg, const int argc);
static void read_options(int argc, char **argv);

int main (int argc, char **argv)
{
        if(!argc) {
                fprintf(stderr, "FATAL: no file passed as argument!\n");
                exit(1);
        }

        int FD;
        char *TargetFile = argv[0];
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
