/**
    UCLA CS 111 - Spring '16
    Lab 2B - Complex Critical Sections
    Arjun 504078752
 **/

#define TRUE 1
#define _GNU_SOURCE
#define CLOCK_TYPE  CLOCK_MONOTONIC_RAW //CLOCK_PROCESS_CPUTIME_ID

#define FILE_MODE       0664
#define SYNC_NONE          0
#define SYNC_ATOMIC        1
#define SYNC_SPINLOCK      2
#define SYNC_PTHREAD_MUTEX 3

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <stdio.h>  /* for fprintf used in debug_log */
#include <stdint.h>
#include <getopt.h> /* Argument Options parse headers */
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#include "SortedList.c"

/* performance-evaluation specific variables */
static volatile long long counter = 0;
static volatile int SPINLOCK = 0;
static pthread_mutex_t MUTEX_LOCK;
static unsigned int sync_type = SYNC_NONE;
static struct timespec start_time,stop_time;

/* option-specific arguments */
int opt_yield  = 0;
static int VERBOSE    = 0;
static unsigned int N_THREADS  = 1;
static unsigned long long ITERATIONS = 1;

static struct option long_options[] = {
        {"sync",       required_argument,         0, 'S'},
        {"yield",      required_argument,         0, 'Y'},
        {"verbose",          no_argument,  &VERBOSE,   1},
        {"threads",    required_argument,         0, 'T'},
        {"iterations", required_argument,         0, 'I'},
};

/* Static function declarations */
static void* count_SYNC_NONE(void *val);
static void* count_SYNC_ATOMIC(void *val);
static void* count_SYNC_SPINLOCK(void *val);
static void* count_SYNC_PTHREAD_MUTEX(void *val);
static void  debug_log(const int opt_index, char **optarg, const int argc);

int main (int argc, char **argv)
{
        int opt=0, opt_index=0;
        long unsigned i;
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
                        ITERATIONS = (atoll(optarg) <= 0) ? 1 : atoll(optarg);
                        if(VERBOSE) debug_log(opt_index, &optarg, 1);
                        break;

                case 'S':
                        if(optarg[0]=='m') sync_type = SYNC_PTHREAD_MUTEX;
                        if(optarg[0]=='s') sync_type = SYNC_SPINLOCK;
                        if(optarg[0]=='c') sync_type = SYNC_ATOMIC;
                        if(VERBOSE) debug_log(opt_index, &optarg, 1);
                        break;

                case 'Y':
		        opt_yield = 1;
                        for(i = 1; i <= strlen(optarg); i++) {
                          if(optarg[0]=='i') opt_yield |= INSERT_YIELD;
                          if(optarg[0]=='d') opt_yield |= DELETE_YIELD;
                          if(optarg[0]=='s') opt_yield |= SEARCH_YIELD;
                        }
                        if(VERBOSE)
                          printf("OPT_YIELD MASK:: %#08x\n", opt_yield);
                        break;
                }
        }

        /** Finished reading all options, begin performance evaluation **/
        void *(*workFunctionPtr)(void *) = count_SYNC_NONE;
        switch(sync_type) {
        case SYNC_ATOMIC:
                workFunctionPtr = count_SYNC_ATOMIC;
                break;

        case SYNC_SPINLOCK:
                workFunctionPtr = count_SYNC_SPINLOCK;
                break;

        case SYNC_PTHREAD_MUTEX:
                if(pthread_mutex_init(&MUTEX_LOCK, NULL)) {
                        fprintf(stderr, "FATAL: Unable to initialize pthread_mutex");
                        exit(1);
                }
                workFunctionPtr = count_SYNC_PTHREAD_MUTEX;
                break;
        }
        /* set long long counter to zero */
        counter = 0;

        /* note the high-res start-time */
        clock_gettime(CLOCK_TYPE, &start_time);

        /* create and start N_THREADS */

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
                if(active_threads[thread_num])
                        pthread_join(thread_pool[thread_num], NULL);

        /* note the high-res ending-time */
        clock_gettime(CLOCK_TYPE, &stop_time);

        /* Close the pthread_mutex if initiated */
        if(sync_type==SYNC_PTHREAD_MUTEX) pthread_mutex_destroy(&MUTEX_LOCK);

        /* output to STDOUT the num_of_operations_performed */
        unsigned long long n_OPS = num_active_threads * ITERATIONS * (2);
        fprintf(stdout, "%d threads x %llu iterations x (add + subtract) = %llu operations\n", num_active_threads, ITERATIONS, n_OPS);

        /* If counter is non-zero, print to STDERR */
        if(VERBOSE || counter != 0)
                fprintf(stderr, "ERROR: final count = %llu\n", counter);

        /* print to STDOUT {elapsed_time, time_per_op} */
        //unsigned long long elapsed_time
        uint64_t elapsed_time = 1000000000L * (stop_time.tv_sec - start_time.tv_sec) + stop_time.tv_nsec - start_time.tv_nsec;
        fprintf(stdout, "elapsed time: %llu ns\n",  (long long unsigned int) elapsed_time);
        fprintf(stdout, "per operation: %llu ns\n", (long long unsigned int) (elapsed_time/n_OPS));

        /* Exit non-zero if counter != 0 */
        exit((counter != 0));
}


/* add function as defined by the spec */
static void add(volatile long long *pointer, long long value) {
        long long sum = *pointer + value;
        if(opt_yield) pthread_yield();
        *pointer = sum;
}

/* Each pthread runs this function NOSYNC */
static void* count_SYNC_NONE(void *val) {
        void* noUse = val;
        unsigned long long i;
        for(i=0; i<ITERATIONS; i++)
        {
                add(&counter,  1);
                add(&counter, -1);
        }
        pthread_exit(NULL);
}

static void* count_SYNC_PTHREAD_MUTEX(void *val) {
        void* noUse = val;
        unsigned long long i;
        pthread_mutex_lock(&MUTEX_LOCK);
        for(i=0; i<ITERATIONS; i++)
        {
                add(&counter,  1);
                add(&counter, -1);
        }
        pthread_mutex_unlock(&MUTEX_LOCK);
        pthread_exit(NULL);
}

static void* count_SYNC_SPINLOCK(void *val) {
        void* noUse = val;
        unsigned long long i;
        while (__sync_lock_test_and_set(&SPINLOCK, 1))
                continue;
        for(i=0; i<ITERATIONS; i++)
        {
                add(&counter,  1);
                add(&counter, -1);
        }
        __sync_lock_release(&SPINLOCK);
        pthread_exit(NULL);
}
static void* count_SYNC_ATOMIC(void *val) {
        void* noUse = val;
        unsigned long long i;
        long long sum, orig;
        for(i=0; i<ITERATIONS; i++) {
                do {
                        orig = counter;
                        if(opt_yield) pthread_yield();
                        sum = orig + 1;
                } while (__sync_val_compare_and_swap(&counter, orig, sum) != orig);;
                do {
                        orig = counter;
                        if(opt_yield) pthread_yield();
                        sum = orig - 1;
                } while (__sync_val_compare_and_swap(&counter, orig, sum) != orig);;
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
