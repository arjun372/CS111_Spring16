/** 
    UCLA CS 111 - Winter '16
    Lab 1 - Simpleton Shell
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
#include "alloc.h"

#define MAX_OPTIONS     26
#define FILE_MODE       0664
#define COMMAND_INDEX   3
#define VERBOSE         FLAGS[0]
#define PROFILE         FLAGS[1]
#define EXIT_STATUS     FLAGS[2]
#define DEBUG           0

typedef struct process {
  pid_t     PID;
  int  argv_off;
  int      argc;
  int    status;
} process_t;

static int FLAGS[5]     = {0};
static int CS_FLAGS[11] = {0};

static char **ARGV = NULL;

static process_t *PROCs  = NULL;
static size_t PROC_reqs  = 0;
static size_t PROCs_size = 0;
static size_t nPROCs     = 0;

static int   *FIDs      = NULL;
static size_t FIDs_size = 0;
static size_t FID_reqs  = 0;

static struct sigaction sVal;
static pid_t  mPID;
static pid_t  gPID;

static struct option long_options[] = {
/* 5 command & file_access options, eval. in switch statement */ 
  {"rdonly",  required_argument, 0, 'r'},
  {"wronly",  required_argument, 0, 'w'},
  {"rdwr",    required_argument, 0, 'd'},
  {"command", required_argument, 0, 'c'},// bugs in cmd arg suppressal
  {"pipe",    no_argument,       0, 'p'},
/* 9 SIGNAL options, evaluated in switch statement  */
  {"abort",   no_argument,         0, 'A'},//
  {"catch",   required_argument,   0, 'H'},//
  {"close",   required_argument,   0, 'C'},   
  {"default", required_argument,   0, 'D'},//
  {"ignore",  required_argument,   0, 'I'},//
  {"wait",    no_argument,         0, 'W'},
  {"pause",   no_argument,         0, 'U'},//
  {"profile", no_argument,   PROFILE,   1},//
  {"verbose", no_argument,   VERBOSE,   1},
/* 11 File creation & status flags, seperate from file access flags */
  {"append",    no_argument, &CS_FLAGS[0],    O_APPEND},
  {"cloexec",   no_argument, &CS_FLAGS[1],   O_CLOEXEC},
  {"creat",     no_argument, &CS_FLAGS[2],     O_CREAT},
  {"directory", no_argument, &CS_FLAGS[3], O_DIRECTORY},
  {"dsync",     no_argument, &CS_FLAGS[4],     O_DSYNC},
  {"excl",      no_argument, &CS_FLAGS[5],      O_EXCL},
  {"nofollow",  no_argument, &CS_FLAGS[6],  O_NOFOLLOW},
  {"nonblock",  no_argument, &CS_FLAGS[7],  O_NONBLOCK},
  {"rsync",     no_argument, &CS_FLAGS[8],     O_RSYNC},
  {"sync",      no_argument, &CS_FLAGS[9],   O_CLOEXEC},
  {"trunc",     no_argument, &CS_FLAGS[10],    O_TRUNC},
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

int main (int argc, char **argv) 
{
  sVal.sa_flags = SA_SIGINFO;
  sVal.sa_sigaction = handleSignal;
  mPID = getpid();
  gPID = getpgid(mPID);

  if(DEBUG) printf("PID,GPID:: %d,%d",mPID,gPID);

  ARGV = argv;
#  __sighandler_t mSIG;
  int i, flag, argcc, opt = 0, opt_index = 0, pipefd[2];
  char **argvc[argc-1];
  char *argi;
  int status;
  pid_t pid;
  size_t IGNORE = 0, N[3],I;
  while(1)
    {
      //      if(IGNORE > 0)
      //	{
      //	  IGNORE-=1;
	  // (**(argv)--);
      /*      if(IGNORE > 0) {
	  (**(argv)--);
	  IGNORE = 0;
      }
      */  //	}
      
      opt = getopt_long_only(argc, argv, "", long_options, &opt_index);
      
      if(opt == -1)
	break;
      
      switch(opt)
	{
	  // Assumption is that Creation & Status flags will have to be set to 0 after this call.
	case 'd':               /* --rdwr   arg1 */
	  flag = O_RDWR;
	  goto fopen_exec;
	case 'w':               /* --wronly arg1 */
	  flag = O_WRONLY;
	  goto fopen_exec;
	case 'r':               /* --rdonly arg1 */
	  flag = O_RDONLY;

	fopen_exec:
	  /* If supplied argument is actually an option, return an error */
	  if(isOption(optarg))
	    goto fopen_err;
	  
	  /* Create mask from all pending Creation & Status Flags */
	  for(i=0;i<11;i++)
	    flag |= CS_FLAGS[i];

	  verbose_log(opt_index, &optarg, 1);

	  /* Open file with supplied flags. If successful, add new FID to array) */
	  if(add_fid(open(optarg, flag, FILE_MODE)) >= 0)
	    goto fopen_good;
	
	fopen_err:
	  EXIT_STATUS = (EXIT_STATUS > 1) ? EXIT_STATUS : 1;
	  error (0, errno, "--%s %s", long_options[opt_index].name, optarg);
	fopen_good:
	  /* Flush all status and creation flags since open call is now done */
	  for(i=0;i<11;i++)
	    CS_FLAGS[i] = 0;
	  break;
	  
	case 'C': /* Close the given FID */
	  
	  /* Check if arg1 is a valid number between (0 -> +INF) */
	  /* Check if arg1 is < total number of FIDs ever allocated */
	  /* Check if FID arg1 is already not closed */
	  if(!(isNumber(optarg) && ((I=atoi(optarg)) < FIDs_size) && (FIDs[I] != -1)))
	    goto fclose_err;

	  verbose_log(opt_index, &optarg, 1);

	  /* FID closed successfully */
	  if(close(FIDs[I]) >= 0)
	    {  
	      FIDs[I] = -1; // Set FID in array to -1;
	      goto fclose_good;
	    }

	fclose_err:
	  EXIT_STATUS = (EXIT_STATUS > 1) ? EXIT_STATUS : 1;
          error (0, errno, "--%s %s", long_options[opt_index].name, optarg);
	fclose_good:
	  break;

	case 'p': /* Open new pipe */
	  flag = (CS_FLAGS[1] | CS_FLAGS[7]);
	  verbose_log(opt_index, &optarg, 0);
	  if(pipe2(pipefd, flag) >= 0 && add_fid(pipefd[0]) && add_fid(pipefd[1]))
	    goto pipe_good;

	pipe_err:
	  EXIT_STATUS = (EXIT_STATUS > 1) ? EXIT_STATUS : 1;
          error (0, errno, "--%s %s", long_options[opt_index].name, optarg);
	pipe_good:
	  CS_FLAGS[1] = CS_FLAGS[7] = 0;
	  break;


	case 'c': /* --command arg1 arg2 arg3 arg4 ..argN {arg1-4 mandatory} */
	  argcc = 0;

	  /* Check # arguments supplied for --command */
	  while(!isOption(argv[(optind-1)+argcc]) && ((optind+argcc++) < argc))
	    argvc[argcc-1] = &argv[(optind-1)+argcc-1];
	  
	  /* Invalid argument size */
	  if (argcc < 4)
	    goto cmd_err;
	    
	  /* check if first 3 args are numbers and valid-usable FIDs */
	  for (i=0;i<3;i++)
	    if(!(isNumber(*argvc[i]) && ((I=atoi(*argvc[i])) < FIDs_size) && (FIDs[I] != -1)))
	      {
		usage(EBADF,long_options[opt_index].name, *argvc[i]);
		goto cmd_exit;
	      }

	  /* Execute command in new child process */
	  I = init_process(optind-1,argcc);
	  flag = execute_command(I);
	  if(flag>0)
	    {
	      PROCs[I].PID = flag;
	      nPROCs++;
	    }
	  goto cmd_good;

	cmd_err:
	      error (0, EINVAL, "--%s %s", "command", argv[(optind-1)]);
	cmd_exit:
	      EXIT_STATUS = (EXIT_STATUS > 1) ? EXIT_STATUS : 1;
	cmd_good:
	      break;

	case 'W': /* WAIT -- Block further execution until processes exit */
	  verbose_log(opt_index, &optarg, 0);
	  while(nPROCs)
	  {
	    /* Wait for all child processes */
	    pid = waitpid(-1, &status, WUNTRACED);
	    for(I=0;I<PROCs_size;I++)
	      if(PROCs[I].PID == pid)
		{
		  /* Set current exit status to highest of children */
		  status = WEXITSTATUS(status);
		  EXIT_STATUS = (status > EXIT_STATUS) ? status : EXIT_STATUS;

		  /* Print out exit status of child process */
		  printf("%d", status);
		  for(i=3;i<PROCs[I].argc;i++)
		    printf(" %s", ARGV[(PROCs[I].argv_off)+i]);
		  printf("\n");
		  break;
		}
	    nPROCs--;
	  }
	  break;

	case 'U': /* Pause until you get a signal */
	  verbose_log(opt_index, &optarg, 0);
	  flag = pause();
	  printf("Paused ended :: %d\n",flag);
	  break;
	
	case 'H': /* Catch signal N and pass it to handler */
	  if(!(isNumber(optarg) && (atoi(optarg) >= 0)))
	     goto ign_err;
	  verbose_log(opt_index, &optarg, 1);
	  sigaction(atoi(optarg), &sVal, NULL);
	  break;
	
	case 'D': /* Default N :: Set default behavior for signal N */
	  mSIG = SIG_DFL;
	  goto ign_exec;
	case 'I': /* Ignore N  :: Ignore signal N */ 
	  mSIG = SIG_IGN;  
	ign_exec:
	  if(!(isNumber(optarg) && (atoi(optarg) >= 0)))
	     goto ign_err;
	  
	  verbose_log(opt_index, &optarg, 1);
	  signal(atoi(optarg), mSIG);
	  goto ign_good;
	  
	ign_err:
	     EXIT_STATUS = (EXIT_STATUS > 1) ? EXIT_STATUS : 1;
	     error (0, errno, "--%s %s", long_options[opt_index].name, optarg);
	ign_good:
	  break;
	
	case 'A': /* Abort now using segfault */
	  killpg(0,SIGSEGV);
	  //abort();
	  break;
	  
	case 0:
	  verbose_log(opt_index, &optarg, 0);
	  break;

	case '?':  
	  printf("? mark\n");
	  break;

	case '@':
	  printf("@ mark\n");
	  break;

	default:
	  printf("default mark\n");
	  break;
	}
    }

  if(FIDs_size)
    free(FIDs);

  if(PROCs_size)
    free(PROCs);
  
  exit(EXIT_STATUS);
}

/* Checks if the given char[] contains numbers only */
static int isNumber(const char* optarg)
{
  int i;
  for(i=0; optarg[i] !='\0'; i++)
      if(optarg[i] < '0' || optarg[i] > '9')
	return 0;
  return 1;
}

/* Add FID to the list of currently usable File Descriptors */
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

/* Add PROC to the list of PROCs */
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

/* Logs to stdout */
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

  /* fork() this process */  
  switch(pid = fork())
    {
      
    case -1: /* fork() Failed */
      usage(errno,long_options[COMMAND_INDEX].name, strerror(errno));
      printf("*** ERROR: forking child process failed\n");
      return -1;

    /* FIRST CHILD's process */  
    case 0:
      if(DEBUG) printf("In Child, PID :: %d\n",pid);
      dup2(FIDs[atoi(argv[0])], STDIN_FILENO);
      dup2(FIDs[atoi(argv[1])], STDOUT_FILENO);
      dup2(FIDs[atoi(argv[2])], STDERR_FILENO);
      execvp(argv[3], &argv[3]);
      if(DEBUG) printf("Child %d Done\n",pid);
      exit(0);

    /* Parent's process */  
    default:
      /* Return FIRST child PID */
      return pid;
    }
}

/* print N caught to stderr */
/* Kill all children */
/* Exit with status N */
static void handleSignal(int sig, siginfo_t *si, void *context)
{
  if(killpg(0,sig)!=0)
    error(0, errno, "unable to kill group %d",gPID);
  error (sig, errno, "%d caught", sig);
}