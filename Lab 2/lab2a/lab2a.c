/**
    UCLA CS 111 - Spring '16
    Lab 2A - Atomic Operations
    Arjun 504078752
**/

#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <error.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>

#define MAX_OPTIONS     26
#define FILE_MODE       0664
#define COMMAND_INDEX   3
#define VERBOSE         1
#define PROFILE         FLAGS[1]
#define DEBUG           0

#define SYNC_NONE          0
#define SYNC_PTHREAD_MUTEX 1
#define SYNC_SPIN_LOCK     2
#define SYNC_ATOMIC        3

typedef struct process {
  pid_t     PID;
  int  argv_off;
  int      argc;
  int    status;
} process_t;

static int correct = 0;
static unsigned long long N_THREADS  = 1;
static unsigned long long ITERATIONS = 1;
static long long counter = 0;
static int opt_yield     = 0;
static int EXIT_STATUS   = 0;

static int sync_version  = SYNC_NONE;
static pthread_mutex_t m_sync_lock;
static volatile int spinlock = 0;
static struct timespec start_time,stop_time;

static int FLAGS[5];
static int CS_FLAGS[11] = {0};

static char **ARGV = NULL;

static process_t *PROCs  = NULL;
static size_t PROC_reqs  = 0;
static size_t PROCs_size = 0;
static size_t nPROCs     = 0;

static int   *FIDs      = NULL;
static size_t FIDs_size = 0;
static size_t FID_reqs  = 0;

static pid_t  mPID;
static pid_t  gPID;

static struct option long_options[] = {
 /* 5 command & file_access options, eval. in switch statement */
  {"threads",     required_argument, 0, 't'},
  {"iterations",  required_argument, 0, 'i'},
  {"yield",    required_argument, 0, 'y'},
  {"correct", no_argument, 0,'c'},
  {"sync", required_argument, 0, 's'},
  {0,0,0,0}

};

static void   verbose_log(const int opt_index, char **optarg, const int argc);
static int    usage (const int errNum, const char *opt_name, const char *optarg);
static int    isOption(const char *opt);
static int    add_fid(const int FID);
static pid_t  execute_command (const size_t proc_index);
static int    isNumber(const char *optarg);
static size_t init_process(const int sargv_offset, const int sargc);
static void   handleSignal(int sig, siginfo_t *si, void *context);


static struct timespec before, after;
void add(long long *pointer, long long value) {

  if(correct)
    {
      clock_gettime(CLOCK_MONOTONIC, &start_time); /* monotonic start time, to remove artifact due to pthread start up */
       //   correct = 0;
    }

   long long sum = *pointer + value;
   if(opt_yield)
     {
       //      clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop_time);
       pthread_yield();
     }
   *pointer = sum;

   if(correct)
      clock_gettime(CLOCK_MONOTONIC, &stop_time); /* monotonic start time */
 }

 /* Each pthread runs this function */
 void * doWork_atomic(void *val){
   size_t i;
   long long sum, orig;
   for(i=0;i<ITERATIONS;i++)
     {
       do
 	{
 	  orig = counter;
 	  sum = orig + 1;
 	} while (__sync_val_compare_and_swap(&counter, orig, sum) != orig);;
       do
 	{
 	  orig = counter;
 	  sum = orig - 1;
 	} while (__sync_val_compare_and_swap(&counter, orig, sum) != orig);;
     }

   pthread_exit(NULL);
 }

 /* Each pthread runs this function */
 void * doWork_mutex(void *val){
   size_t i;
   pthread_mutex_lock(&m_sync_lock);
   for(i=0;i<ITERATIONS;i++)
     {
       add(&counter,1);
       add(&counter,-1);
     }
   pthread_mutex_unlock(&m_sync_lock);
   pthread_exit(NULL);
 }

 /* Each pthread runs this function */
 void * doWork_spinlock(void *val){
   size_t i;

   while (__sync_lock_test_and_set(&spinlock, 1))
     continue;
   for(i=0;i<ITERATIONS;i++)
     {
       add(&counter,1);
       add(&counter,-1);
     }
   __sync_lock_release(&spinlock);
   pthread_exit(NULL);
 }

 /* Each pthread runs this function */
 void * doWork(void *val){
   size_t i;
   for(i=0;i<ITERATIONS;i++)
     {
       add(&counter,1);
       add(&counter,-1);
     }
   pthread_exit(NULL);
 }


 #define BILLION 1000000000L

 int localpid(void) {
   static int a[9] = { 0 };
   return a[0];
 }

 int main (int argc, char **argv)
 {
   ARGV = argv;
   int i, flag, argcc, opt = 0, opt_index = 0, pipefd[2];
   char **argvc[argc-1];
   char *argi;
   int status;
   pid_t pid;
   size_t IGNORE = 0, N[3],I;
   while(1)
     {
       opt = getopt_long_only(argc, argv, "", long_options, &opt_index);

       if(opt == -1)
 		break;

       switch(opt)
 	{
 	case 'c':
 	  correct = 1;
 	  break;

 	case 't':
 	case 'i':
 	  /* If supplied argument is actually an option, return an error */
 	  if(!isNumber(optarg) || ((I=atoi(optarg)) < 1))
 	    goto fopen_err;

	/*Set N_THREADS = optarg*/
	  verbose_log(opt_index, &optarg, 1);
	  if(opt == 't')
	    N_THREADS = I;
	  else
	    ITERATIONS = I;

	  break;

	case 'y':
	  if(!isNumber(optarg) || ((i=atoi(optarg)) < 0 ) || ((i=atoi(optarg)) > 1 ))
	    goto fopen_err;

	  verbose_log(opt_index, &optarg, 1);
	  opt_yield = i;
	  break;

        case 's':
	  if(optarg[0]=='m')
	    sync_version = SYNC_PTHREAD_MUTEX;
	  else if(optarg[0]=='s')
	    sync_version = SYNC_SPIN_LOCK;
	  else if(optarg[0]=='c')
            sync_version = SYNC_ATOMIC;
	  else
	    sync_version = SYNC_NONE;

	  verbose_log(opt_index, &optarg, 1);
	  break;
	fopen_err:
	  error (0, errno, "--%s %s", long_options[opt_index].name, optarg);
	  break;

	default:
	  printf("default mark\n");
	  break;
	}
    }

  void *(*workFunctionPtr)(void *);

  uint64_t artifact_delay, diff, real_diff;
  pthread_t thread_pool[N_THREADS];
  unsigned int validThreads[N_THREADS];
  unsigned long long  R_THREADS = 0;
  unsigned long long  worker_n;
  counter = 0;

  switch(sync_version)
    {
    case SYNC_ATOMIC:
      workFunctionPtr = doWork_atomic;
      break;

    case SYNC_SPIN_LOCK:
      workFunctionPtr = doWork_spinlock;
      break;

    case SYNC_PTHREAD_MUTEX:
      if(pthread_mutex_init(&m_sync_lock, NULL)) goto bad_things_happen_all_the_time;
      workFunctionPtr = doWork_mutex;
      break;

    default:
    case SYNC_NONE:
      workFunctionPtr = doWork;
      break;
    }

  if(!correct)
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);  /*monotonic start time*/

  ///  clock_gettime(CLOCK_MONOTONIC, &start_time);
  // clock_gettime(CLOCK_MONOTONIC, &stop_time);
  //artifact_delay = stop_time.tv_nsec - start_time.tv_nsec;

  /* Create N new threads */
  for(worker_n = 0; worker_n < N_THREADS; worker_n++)
    {
      if(pthread_create(&thread_pool[worker_n], NULL, workFunctionPtr, (void *)(NULL)) == 0)
	{
	  R_THREADS++;
	  validThreads[worker_n] = 1;
	}
      else
	validThreads[worker_n] = 0;
    }

  unsigned long long n_OPS = R_THREADS * ITERATIONS * (2);
  printf("%llu threads x %llu iterations x (add + subtract) = %llu operations\n",R_THREADS,ITERATIONS, n_OPS);

  //Wait for all threads to join   TODO :: CHECK IF R or N THREADS
  for(worker_n=0; worker_n < N_THREADS; worker_n++)
    if(validThreads[worker_n] == 1)
      pthread_join(thread_pool[worker_n], NULL);

  if(!correct)
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop_time);

  if(sync_version==SYNC_PTHREAD_MUTEX)
    pthread_mutex_destroy(&m_sync_lock);

  if(counter)
    {
      printf("ERROR: final count = %llu\n", counter);
      EXIT_STATUS = 1;
    }

  diff = stop_time.tv_nsec - start_time.tv_nsec;

  //printf("elapsed process CPU time = %llu nanoseconds\n", (long long unsigned int) diff);
  //  printf("artifact_delay: %llu ns\n", (long long unsigned int) artifact_delay);
  printf("elapsed time: %llu ns\n", (long long unsigned int) diff);
  printf("per operation: %llu ns\n",(long long unsigned int) diff/n_OPS);

 exit(EXIT_STATUS);
 bad_things_happen_all_the_time:
 printf("pthread_mutex_init FAILED\n");
 exit(1);
}

// Checks if the given char[] contains numbers only
static int isNumber(const char* optarg)
{
  int i;
  for(i=0; optarg[i] !='\0'; i++)
      if(optarg[i] < '0' || optarg[i] > '9')
	return 0;
  return 1;
}

//Add FID to the list of currently usable File Descriptors
static int add_fid(const int FID)
{
  if(FID<0)
    goto exit;

  if(!FIDs_size)
    FIDs = (int *)checked_malloc(sizeof(int)*(++FIDs_size));
  else
    FIDs = (int *)checked_realloc(FIDs, sizeof(int)*(++FIDs_size));

  FIDs[FID_reqs++] = FID;

 exit:
  return FID;
}

//Add PROC to the list of PROCs
static size_t init_process(const int sargv_off, const int sargc)
{
  process_t nPROC = {.PID = -1, .argv_off = sargv_off, .argc = sargc, .status = -1};

  if(!PROCs_size)
    PROCs = (process_t *)checked_malloc(sizeof(process_t)*(++PROCs_size));
  else
    PROCs = (process_t *)checked_realloc(PROCs, sizeof(process_t)*(++PROCs_size));

  PROCs[PROC_reqs] = nPROC;
  return (PROC_reqs++);
}


/* Checks if the givent string is a valid option as defined by the spec.    *
 * Helps in checking whether arguments passed to options are valid or not. */
static int isOption(const char *opt)
{
  int i;
  for(i=0; i<(MAX_OPTIONS-1); i++)
    {
      char str[20];
      strcpy(str, "--");
      strcat(str, long_options[i].name);
      if(strcmp(opt,str) == 0)
        return 1;
    }
  return 0;
}

/* Logs to stdout  */
static void verbose_log(const int opt_index, char **optarg, const int argc)
{
  if(!VERBOSE)
    return;

  int i;
  printf("--%s", long_options[opt_index].name);
  for(i = 0; i < argc; i++)
    printf(" %s", optarg[i]);
  printf("\n");
}

/* Reports errors to stderr, along with exit code */
static int usage (const int errNum, const char *opt_name, const char *optarg)
{
  EXIT_STATUS = (errNum > EXIT_STATUS) ? errNum : EXIT_STATUS;
  error (0, errNum, "--%s %s", opt_name, optarg);
  return EXIT_STATUS;
}

static pid_t execute_command (const size_t proc_index)
{
  pid_t pid    = PROCs[proc_index].PID;
  int status   = PROCs[proc_index].status;
  int argv_off = PROCs[proc_index].argv_off;
  int argc     = PROCs[proc_index].argc;
  int i;

  /* Verbose log and assignment */
  char* argv[argc+1];
  argv[argc] = (char*) NULL;
  printf("--%s", long_options[COMMAND_INDEX].name);
  for(i=0;i<argc;i++)
    {
      argv[i] = ARGV[argv_off+i];
      printf(" %s",argv[i]);
    }
  printf("\n");

  if(DEBUG) printf("Parent PID:: %d\n",getpid());

  //  fork() this process
  switch(pid = fork())
    {

    case -1: // fork() Failed
      usage(errno,long_options[COMMAND_INDEX].name, strerror(errno));
      printf("*** ERROR: forking child process failed\n");
      return -1;

     /*FIRST CHILD's process*/
    case 0:
      if(DEBUG) printf("In Child, PID :: %d\n",pid);
      dup2(FIDs[atoi(argv[0])], STDIN_FILENO);
      dup2(FIDs[atoi(argv[1])], STDOUT_FILENO);
      dup2(FIDs[atoi(argv[2])], STDERR_FILENO);
      execvp(argv[3], &argv[3]);
      if(DEBUG) printf("Child %d Done\n",pid);
      exit(0);

     /*Parent's process*/
    default:
      //Return FIRST child PID
      return pid;
    }
}

 /*print N caught to stderr
 Kill all children
 Exit with status N */
static void handleSignal(int sig, siginfo_t *si, void *context)
{
  if(killpg(0,sig)!=0)
    error(0, errno, "unable to kill group %d",gPID);
  error (sig, errno, "%d caught", sig);
}
