/**
    UCLA CS111 - Spring '16
    Arjun 504078752
    Lab 1a
**/

#define N_THREADS 1
#define TRUE      1
#define FALSE     0
#define NRAW_ERR  1
#define OPEN_ERR  1
#define CREA_ERR  2
#define CATC_ERR  3
#define FILE_MOD  0664

#define _GNU_SOURCE /* for pipe2 */

/* To get & set Terminal attributes */
#include <termios.h>
/* Argument Options parse headers */
#include <getopt.h>
/* Error handling include headers */
#include <error.h>
#include <errno.h>
/* Stdio function headers */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
/* open(2) include headers */
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
/* signal(2) include headers */
#include <signal.h>
/* read(2), write(2) include headers */
#include <unistd.h>
/* pthreads include headers */
#include <pthread.h>

static struct termios TERM_INIT;
static char CR   = 0x0D;
static char LF   = 0x0A;
static char mEOF = 0x04;
static char mSIGINT = 0x03;

static pid_t CHILD_PID = -2;
static int VERBOSE = 0;
static int SHELL   = 0;
static char *ptr = NULL;

/* pthread Worker Pool */
static pthread_t thread_pool[N_THREADS];
static unsigned int validThreads[N_THREADS];
static unsigned long long  R_THREADS = 0;
static unsigned long long  worker_n;

static int I_PIPE_FD[2] = {-1, -1};
static int O_PIPE_FD[2] = {-1, -1};

static struct option long_options[] = {
  {"shell",   no_argument,   &SHELL, 1},
  {"verbose", no_argument, &VERBOSE, 1},
};

/* function definitions */
static void setTC_initial();
static void setTC_cooked();
static void basic_mode();
static void shell_mode();
static void sig_handler(int sigN);

/* pThread worker functions */
void *doWork_SHELL_2_STDOUT(void *val);

/* To be run at exit */
int main(int argc, char **argv) {

 while(getopt_long_only(argc, argv, "", long_options, NULL) != -1)
  continue;

 /* STEP 13/13 :: Save current terminal settings */
 /* STEP 13/13 :: atexit, set terminal to initial/cooked mode */
 if(tcgetattr(STDIN_FILENO, &TERM_INIT) == 0)
  atexit(setTC_initial);
 else
  {
    if(VERBOSE) fprintf(stderr, "Unable to get tty attr, will set to COOKED on exit.\n");
    atexit(setTC_cooked);
  }

 /* Step 1/13 :: Put console into RAW mode */
 struct termios term_raw = TERM_INIT;
 cfmakeraw(&term_raw);
 term_raw.c_cc[VMIN]  = 1;
 term_raw.c_cc[VTIME] = 0;
 term_raw.c_lflag &= ~ISIG;

 if(tcsetattr(STDIN_FILENO, TCSANOW, &term_raw) != 0)
   fprintf(stderr, "FATAL :: Unable to switch to RAW mode, no guanrantees from this point on!\n");

 if(SHELL)
  shell_mode();
 else
  basic_mode();

  exit(0);
}

static void shell_mode() {

  /* Step 8/12 :: Catch SIGINT and forward it to shell */
  if(signal(SIGINT, sig_handler) == SIG_ERR)
      fprintf(stderr, "ERROR(%d): unable to catch SIGINT\n", errno);

  // TODO :: Only child should be able to send you pipe!
  if(signal(SIGPIPE, sig_handler) == SIG_ERR)
    fprintf(stderr, "ERROR(%d): unable to catch SIGPIPE\n", errno);

  char * cmds[] = {"/bin/bash", NULL};
  int byte_written, S_BYTE, P_BYTE;

  /* Create 2 pipes */
  if(pipe2(I_PIPE_FD, 0) < 0 || pipe2(O_PIPE_FD, 0) < 0) /* Creating pipe failed */
    exit(2);

  /* Create N new threads */
  void *(*workFunctionPtr)(void *);
  workFunctionPtr = doWork_SHELL_2_STDOUT;
  for(worker_n = 0; worker_n < N_THREADS; worker_n++)
    {
      if(pthread_create(&thread_pool[worker_n], NULL, workFunctionPtr, (void *)(NULL)) == 0)
  	   {
  	      R_THREADS++;
  	      validThreads[worker_n] = 1;
  	   }
      else validThreads[worker_n] = 0;
    }

  pid_t pPID = getpid();
  if(VERBOSE) fprintf(stderr, "Parent PID : %d\n", pPID);

  /* Step 4/13 :: Fork a new process */
  switch(pPID = fork())
    {
      case -1 : /* fork() failed */
      fprintf(stderr, "FATAL: error forking child process\n");
      exit(2);

      case 0 : /* In child's process */
      if(VERBOSE) fprintf(stderr, "Child PID : %d\n", getpid());
      dup2(O_PIPE_FD[0], STDIN_FILENO);
      dup2(I_PIPE_FD[1], STDOUT_FILENO);
      dup2(I_PIPE_FD[1], STDERR_FILENO);
      close(I_PIPE_FD[0]);
      execvp(cmds[0], cmds);
      break;

      default:
        CHILD_PID = pPID;
        close(I_PIPE_FD[1]);
        /* Step 2/13 :: Read char-by-char   */
        /* Step 3/13 :: Handles long inputs */
        /* Step 5/13 :: write char-by-char  */
        while(read(STDIN_FILENO, &S_BYTE, 1))
        {
            if(S_BYTE == mSIGINT)
            {
              if(VERBOSE) fprintf(stderr,"CAUGHT SIGNAL %d, forwarding to CHILD %d\n", mSIGINT, CHILD_PID);
              kill(CHILD_PID, SIGINT);
            }

            /* Step 9/12 :: upon EOF, close pipe, send SIGHUP, restore & exit(0) */
            if(S_BYTE == mEOF)
            {
                close(O_PIPE_FD[0]);
                close(O_PIPE_FD[1]);
                kill(CHILD_PID, SIGHUP);
                exit(0);
            }

            /* Step 7/12 :: Pipe to shell, NO ECHO on PIPE */
            if(S_BYTE == CR || S_BYTE == LF)
            {
              byte_written = write(STDOUT_FILENO, &CR, 1);
              S_BYTE = LF;
            }

            byte_written = write(STDOUT_FILENO, &S_BYTE, 1);
            byte_written = write(O_PIPE_FD[1], &S_BYTE, 1);
        }
        break;
    }
  return;
}
static void basic_mode() {
 /* Step 2/13 :: Read char-by-char   */
 /* Step 3/13 :: Handles long inputs */
 /* Step 5/13 :: write char-by-char  */
 int byte_written;
 char BYTE;
 while(read(STDIN_FILENO, &BYTE, 1))
 {
   if(BYTE == mEOF)
     break;

   if(BYTE == CR || BYTE == LF)
   {
     byte_written = write(STDOUT_FILENO, &CR, 1);
     BYTE = LF;
   }
   byte_written = write(STDOUT_FILENO, &BYTE, 1);
 }
}

static void sig_handler(int sigN) {
  switch (sigN)
  {
    case SIGINT:
      if(VERBOSE) fprintf(stderr,"CAUGHT SIGNAL %d, forwarding to CHILD %d\n", sigN, CHILD_PID);
      kill(CHILD_PID, sigN);
      break;

    case SIGPIPE:
      if(VERBOSE) fprintf(stderr,"CAUGHT SIGNAL %d from CHILD %d\n", sigN, CHILD_PID);
      exit(1);
  }

}

static void setTC_initial() {
  if(VERBOSE)fprintf(stderr, "ON EXIT:: Set INITIAL mode....");
  if(tcsetattr(STDIN_FILENO, TCSANOW, &TERM_INIT) == 0)
    {if(VERBOSE)fprintf(stderr, "SUCCESS\n");}
  else
    {if(VERBOSE)fprintf(stderr, "FAILED\n");}

  if(SHELL)
  {
    int child_status = 8888;
    waitpid(CHILD_PID, &child_status, 0);
    printf("Child exited with status : %d\n", WEXITSTATUS(child_status));
  }
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
}

static void setTC_cooked() {
  struct termios term_cooked = {
    .c_iflag = (TERM_INIT.c_iflag | IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON ),
    .c_cflag = (TERM_INIT.c_cflag | OPOST ),
    .c_oflag = (TERM_INIT.c_oflag | ECHO | ECHONL | ICANON | ISIG | IEXTEN ),
    .c_lflag = (TERM_INIT.c_lflag | CSIZE | PARENB ),
  };
  if(VERBOSE)fprintf(stderr, "ON EXIT:: Set COOKED mode.\n");
  if(tcsetattr(STDIN_FILENO, TCSANOW, &term_cooked) == 0)
    {if(VERBOSE)fprintf(stderr, "SUCCESS\n");}
  else
    {if(VERBOSE)fprintf(stderr, "FAILED\n");}

    if(SHELL)
    {
      int child_status = 8888;
      waitpid(CHILD_PID, &child_status, 0);
      printf("Child exited with status : %d\n", WEXITSTATUS(child_status));
    }
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
}

/* Step 6/13 :: Do work in separate thread */
void * doWork_SHELL_2_STDOUT(void *val) {
  int byte_written, P_BYTE;
  void *x = val;
  while(read(I_PIPE_FD[0], &P_BYTE, 1))
    {
      if(P_BYTE == mEOF)
      {
        if(VERBOSE)fprintf(stderr, "EOF from shell, exiting (1)\n");
        exit(1);
      }
      byte_written = write(STDOUT_FILENO, &P_BYTE, 1);
    }
    if(VERBOSE)fprintf(stderr, "EOF from shell, exiting (1)\n");
    exit(1);

  pthread_exit(NULL);
}
