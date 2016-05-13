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
pos_info superblock[] = {
  {"magic number", "%h", 1, 1}
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
        }

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
