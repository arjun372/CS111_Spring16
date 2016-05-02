/**
    UCLA CS 111 - Spring '16
    Lab 2B - Complex Critical Sections
    Arjun 504078752
 **/

#define _GNU_SOURCE

#define KEY_SIZE             10

#define SYNC_NONE          0x00
#define SYNC_SPINLOCK      0x01
#define SYNC_PTHREAD_MUTEX 0x02

#define CLOCK_TYPE  CLOCK_MONOTONIC_RAW   /* CLOCK_PROCESS_CPUTIME_ID */

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
static SortedList_t SharedList;
static SortedListElement_t *Nodes;
static char **Keys;

/* performance-evaluation specific variables */
static volatile long long counter = 0;
static volatile int SPINLOCK = 0;
static pthread_mutex_t MUTEX_LOCK;
static unsigned int sync_type = SYNC_NONE;
static struct timespec start_time,stop_time;

/* option-specific variables */
int opt_yield = 0;
static int VERBOSE = 0;
static unsigned int N_THREADS = 1;
static unsigned long long ITERATIONS = 1;

static struct option long_options[] = {
        {"sync",       required_argument,         0, 'S'},
        {"yield",      required_argument,         0, 'Y'},
        {"verbose",          no_argument,  &VERBOSE,   1},
        {"threads",    required_argument,         0, 'T'},
        {"iterations", required_argument,         0, 'I'},
};

/* Static function declarations */
static void *doWork(void *offset);
static char *alloc_rand_string(const long long unsigned size);
static void debug_log(const int opt_index, char **optarg, const int argc);
static SortedListElement_t *init_and_fill(const long long unsigned nBlocks);

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
                        if(VERBOSE) debug_log(opt_index, &optarg, 1);
                        break;

                case 'Y':
                        for(i = 0; i < strlen(optarg); i++) {
                                if(optarg[i]=='i') opt_yield |= INSERT_YIELD;
                                if(optarg[i]=='d') opt_yield |= DELETE_YIELD;
                                if(optarg[i]=='s') opt_yield |= SEARCH_YIELD;
                        }
                        if(!VERBOSE)
                                break;
                        printf("OPT_YIELD MASK:: %#08x\n", opt_yield);
                        if(opt_yield & INSERT_YIELD) printf("INSERT YIELD!!\n");
                        if(opt_yield & SEARCH_YIELD) printf("SEARCH YIELD!!\n");
                        if(opt_yield & DELETE_YIELD) printf("DELETE YIELD!!\n");
                        break;
                }
        }

        /** Finished reading all options, begin performance evaluation **/

        /* initialize mutex */
        if(sync_type==SYNC_PTHREAD_MUTEX)
                if(pthread_mutex_init(&MUTEX_LOCK,NULL))
                {
                        fprintf(stderr, "FATAL: Unable to initialize pthread_mutex");
                        exit(1);
                }

        /* initialize an empty list */
        SharedList.prev = NULL;
        SharedList.next = NULL;
        SharedList.key  = NULL;

        /* create & initialize (iteration * thread) of list elements */
        Nodes = init_and_fill(N_THREADS * ITERATIONS);

        /* Allocate memory for N_THREADS and corresponding operations */
        unsigned int thread_num;
        pthread_t thread_pool[N_THREADS];
        unsigned int num_active_threads = 0;
        unsigned int active_threads[N_THREADS];

        /* note the high-res start-time */
        clock_gettime(CLOCK_TYPE, &start_time);

        /* create and start N_THREADS */
        for(thread_num = 0; thread_num < N_THREADS; thread_num++)
        {
                /* Allocate memory for each thread's argument list */
                unsigned int *arg = malloc(sizeof(*arg));
                if (arg == NULL) {
                        fprintf(stderr, "Couldn't allocate memory for thread arg.\n");
                        exit(EXIT_FAILURE);
                }
                *arg = thread_num;
                if(pthread_create(&thread_pool[thread_num], NULL, doWork, arg) == 0)
                {
                        num_active_threads++;
                        active_threads[thread_num] = 1;
                }
                else
                {
                        active_threads[thread_num] = 0;
                        free(arg);
                }
        }

        /* wait for active_threads to join */
        for(thread_num = 0; thread_num < N_THREADS; thread_num++)
                if(active_threads[thread_num])
                        pthread_join(thread_pool[thread_num], NULL);

        /* note the high-res ending-time */
        clock_gettime(CLOCK_TYPE, &stop_time);

        /* Close the pthread_mutex if initiated */
        if(sync_type == SYNC_PTHREAD_MUTEX) pthread_mutex_destroy(&MUTEX_LOCK);

        /* output to STDOUT the num_of_operations_performed */
        unsigned long long n_OPS = num_active_threads * ITERATIONS * (2);
        fprintf(stdout, "%d threads x %llu iterations x (insert + lookup/delete) = %llu operations\n", num_active_threads, ITERATIONS, n_OPS);

        /* If counter is non-zero, print to STDERR */
        int list_length = SortedList_length(&SharedList);
        if(VERBOSE || list_length != 0)
                fprintf(stderr, "FINAL :: list_length = %d\n", list_length);

        /* print to STDOUT {elapsed_time, time_per_op} */
        uint64_t elapsed_time = 1000000000L * (stop_time.tv_sec - start_time.tv_sec) + stop_time.tv_nsec - start_time.tv_nsec;
        fprintf(stdout, "elapsed time: %llu ns\n",  (long long unsigned int) elapsed_time);
        fprintf(stdout, "per operation: %llu ns\n", (long long unsigned int) (elapsed_time/n_OPS));

        free(Nodes);
        for(i = 0; i < (N_THREADS * ITERATIONS); i++)
                free(Keys[i]);
        free(Keys);
        /* Exit non-zero if list_length != 0 */
        exit((list_length != 0));
}

/* Run OPs per thread */
static void *doWork(void *offset) {

        /* Get offset and free the memory */
        unsigned int i = *((unsigned int *) offset);
        free(offset);

        /* Calculate the start & stop offsets in Keys */
        unsigned int start = i * ITERATIONS;
        unsigned int j     = start;
        unsigned int stop  = ITERATIONS + start - 1;

        /* Add thread_local elements Nodes[start:stop] into SharedList */
        if(sync_type == SYNC_PTHREAD_MUTEX)
                pthread_mutex_lock(&MUTEX_LOCK);
        else if(sync_type == SYNC_SPINLOCK)
                while (__sync_lock_test_and_set(&SPINLOCK, 1))
                        continue;
        for(j = start; j <= stop; j++)
                SortedList_insert(&SharedList, &(Nodes[j]));

        /* get SharedList length */
        int len = SortedList_length(&SharedList);
        if(VERBOSE) fprintf(stderr, "Thread %d : list_length : %d\n", i, len);

        /* Lookup each element with known key and delete it */
        for(j = start; j <= stop; j++)
                SortedList_delete(SortedList_lookup(&SharedList, Keys[j]));

        if(sync_type == SYNC_PTHREAD_MUTEX)
                pthread_mutex_lock(&MUTEX_LOCK);
        else if(sync_type == SYNC_SPINLOCK)
                __sync_lock_release(&SPINLOCK);

        pthread_exit(NULL);
}

static SortedListElement_t *init_and_fill(const long long unsigned nBlocks) {

        /* Initializes the elements */
        SortedListElement_t *elements = (SortedListElement_t *) malloc(sizeof(SortedListElement_t) * nBlocks);
        if(elements == NULL) goto MemErr;

        /* Initialize a large list of char pointers to keep track of allocated memory */
        Keys = (char **) malloc(nBlocks * sizeof(char *));
        if(Keys == NULL) goto MemErr;

        /* Fill each element with random data */
        long long unsigned iterator;
        for(iterator = 0; iterator < nBlocks; iterator++) {
                Keys[iterator] = alloc_rand_string(KEY_SIZE);
                elements[iterator].key = Keys[iterator];
        }

        return elements;
MemErr:
        fprintf(stderr, "FATAL:: Unable to allocate memory\n");
        free(elements);
        free(Keys);
        exit(1);
}

/* Allocate a random string of @param size */
static char* alloc_rand_string(const long long unsigned size) {
        const char charset[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        char *str = malloc(sizeof(char) * (size + 1));
        if(str == NULL) goto MemErr;

        int offset;
        str[size] = '\0';
        unsigned long long i;
        for (i = 0; i < size; i++) {
                offset = rand() % (int) (sizeof(charset) - 1);
                str[i] = charset[offset];
        }
        return str;
MemErr:
        fprintf(stderr, "FATAL:: Unable to allocate memory for key\n");
        free(str);
        exit(1);
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
