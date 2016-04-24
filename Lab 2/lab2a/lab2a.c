/**
    UCLA CS 111 - Spring '16
    Lab 2A - Atomic Operations
    Arjun 504078752
**/
#define TRUE 1

#include <getopt.h> /* Argument Options parse headers */
#include <stdlib.h>
#include <string.h> /* for fprintf used in debug_log */


static int YIELD      = 0;
static int VERBOSE    = 0;
static int N_THREADS  = 1;
static int ITERATIONS = 1;

static struct option long_options[] = {
  {"sync",       required_argument,         0, 'S'},
  {"yield",            no_argument,    &YIELD,   1},
  {"threads",    required_argument,         0, 'T'},
  {"verbose",          no_argument,  &VERBOSE,   1},
  {"iterations", required_argument,         0, 'I'},
};

/* Static function declarations */
static void debug_log(const int opt_index, char **optarg, const int argc);

int main (int argc, char **argv)
{
  int opt=0, opt_index=0;
  while(TRUE)
    {
      opt = getopt_long_only(argc, argv, "", long_options, &opt_index);

      /* All options have been read */
      if(opt==-1)
        break;

      switch(opt) {
        case 'T':
          N_THREADS = atoi(optarg);
          if(VERBOSE) debug_log(opt_index, &optarg, 1);
          break;

        case 'I':
          ITERATIONS = atoi(optarg);
          if(VERBOSE) debug_log(opt_index, &optarg, 1);
          break;
      }
    }

  /** Finished reading all options, begin performance evaluation **/
  exit(0);
}


/* if --VERBOSE is passed, logs to stdout */
static void debug_log(const int opt_index, char **optarg, const int argc) {
  if(!VERBOSE)
    return;

  int i;
  fprintf(stderr,"--%s", long_options[opt_index].name);
  for(i = 0; i < argc; i++)
    fprintf(stderr," %s", optarg[i]);
  fprintf(stderr,"\n");
}
