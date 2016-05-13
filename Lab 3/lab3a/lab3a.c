/**
    UCLA CS 111 - Spring '16
    Lab 3A - File System Dump
    Rahul Malavalli 204429252
    Arjun           504078752
**/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>  /* for fprintf used in debug_log */
#include <stdint.h>
#include <getopt.h> /* Argument Options parse headers */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


/* performance-evaluation specific variables */
static unsigned int sync_type = SYNC_NONE;
static struct timespec start_time,stop_time;

/* option-specific variables */
static int VERBOSE = 0;
static struct option long_options[] = {
        {"verbose",          no_argument,  &VERBOSE,   1},
};

/* Static function declarations */
static void debug_log(const int opt_index, char **optarg, const int argc);
static void read_options(int argc, char **argv);

int main (int argc, char **argv)
{

}

/* if --VERBOSE is passed, logs to stdout */
static void debug_log(const int opt_index, char **optarg, const int argc) {
        fprintf(stderr,"--%s", long_options[opt_index].name);
        int i;
        for(i = 0; i < argc; i++)
                fprintf(stderr," %s", optarg[i]);
        fprintf(stderr,"\n");
}
