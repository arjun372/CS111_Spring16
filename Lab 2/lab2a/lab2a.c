/**
    UCLA CS 111 - Spring '16
    Lab 2A - Atomic Operations
    Arjun 504078752
**/
#define TRUE 1
#define CLOCK_TYPE CLOCK_MONOTONIC_RAW

#include <time.h>
#include <stdio.h>  /* for fprintf used in debug_log */
#include <getopt.h> /* Argument Options parse headers */
#include <stdlib.h>
#include <pthread.h>



static long long counter = 0;


/* option-specific arguments */
static int YIELD      = 0;
static int VERBOSE    = 0;
static unsigned int N_THREADS  = 1;
static unsigned long long ITERATIONS = 1;

static struct option long_options[] = {
  {"sync",       required_argument,         0, 'S'},
  {"yield",            no_argument,    &YIELD,   1},
  {"threads",    required_argument,         0, 'T'},
  {"verbose",          no_argument,  &VERBOSE,   1},
  {"iterations", required_argument,         0, 'I'},
};

/* Static function declarations */
static void  debug_log(const int opt_index, char **optarg, const int argc);
static void* count_NOSYNC(void *val);

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
          N_THREADS = (atoi(optarg) <= 0) ? 1 : atoi(optarg);
          if(VERBOSE) debug_log(opt_index, &optarg, 1);
          break;

        case 'I':
          ITERATIONS = (atoi(optarg) <= 0) ? 1 : atoi(optarg);
          if(VERBOSE) debug_log(opt_index, &optarg, 1);
          break;
      }
    }

  /** Finished reading all options, begin performance evaluation **/

  /* set long long counter to zero */
  counter = 0;

  /* note the high-res start-time */
  struct timespec start_time,stop_time;
  clock_gettime(CLOCK_TYPE, &start_time);

  /* create and start N_THREADS */
  void *(*workFunctionPtr)(void *) = count_NOSYNC;
  unsigned int active_threads[N_THREADS];
  unsigned int num_active_threads = 0;
  pthread_t thread_pool[N_THREADS];
  unsigned int thread_num;
  for(thread_num=0; thread_num<N_THREADS; thread_num++)
  {
    if(pthread_create(&thread_pool[thread_num], NULL, workFunctionPtr, (void *)(NULL)) == 0)
    {
      num_active_threads++;
      active_threads[thread_num] = 1;
    }
    else
      active_threads[thread_num] = 0;
  }

  /* wait for active_threads to join */
  for(thread_num=0; thread_num<N_THREADS; thread_num++)
    if(active_threads[thread_num] == 1)
      pthread_join(thread_pool[thread_num], NULL);

  /* note the high-res ending-time */
  clock_gettime(CLOCK_TYPE, &stop_time);

  /* output to STDOUT the num_of_operations_performed */
  unsigned long long n_OPS = num_active_threads * ITERATIONS * (2);
  fprintf(stdout, "%d threads x %llu iterations x (add + subtract) = %llu operations\n", num_active_threads, ITERATIONS, n_OPS);

  /* If counter is non-zero, print to STDERR */
  if(VERBOSE || counter != 0)
    fprintf(stderr, "ERROR: final count = %llu\n", counter);

  /* print to STDOUT {elapsed_time, time_per_op} */
  unsigned long long elapsed_time = stop_time.tv_nsec - start_time.tv_nsec;
  fprintf(stdout, "elapsed time: %llu ns\n", elapsed_time);
  fprintf(stdout, "per operation: %llu ns\n", (elapsed_time/n_OPS));

  /* Exit non-zero if counter != 0 */
  exit((counter != 0));
}

/* add function as defined by the spec */
void add(long long *pointer, long long value) {
  long long sum = *pointer + value;
  *pointer = sum;
}

/* Each pthread runs this function NOSYNC */
static void* count_NOSYNC(void *val) {
  unsigned long long i;
  void* noUse = val;
  for(i=0;i<ITERATIONS;i++)
    {
      add(&counter,  1);
      add(&counter, -1);
    }
  pthread_exit(NULL);
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
