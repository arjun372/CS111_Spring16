/**
    UCLA CS111 - Spring '16
    Arjun 504078752
    Lab 1a
**/

#define TRUE     1
#define FALSE    0
#define NRAW_ERR 1
#define OPEN_ERR 1
#define CREA_ERR 2
#define CATC_ERR 3
#define FILE_MOD 0664

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
#include <sys/stat.h>
#include <fcntl.h>
/* signal(2) include headers */
#include <signal.h>
/* read(2), write(2) include headers */
#include <unistd.h>


static struct termios TERM_INIT;
static char CR   = 0x0D;
static char LF   = 0x0A;
static char mEOF = 0x04;
static char BYTE;

static int VERBOSE = 0;
static int SHELL   = 0;
static char *ptr = NULL;

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
void sig_handler(int sigN);

/* To be run at exit */
int main(int argc, char **argv) {

 while(getopt_long_only(argc, argv, "", long_options, NULL) != -1)
  continue;

 /* STEP 1/12 :: Save current terminal settings */
 /* STEP 2/12 :: atexit, set terminal to initial/cooked mode */
 if(tcgetattr(STDIN_FILENO, &TERM_INIT) == 0)
  atexit(setTC_initial);
 else
  {
    fprintf(stderr, "Unable to get tty attr, will set to COOKED on exit.\n");
    atexit(setTC_cooked);
  }

 /* Step 3/12 :: Put console into RAW mode */
 struct termios term_raw = TERM_INIT;
 cfmakeraw(&term_raw);
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

  if(signal(SIGINT, sig_handler) == SIG_ERR)
    fprintf(stderr, "ERROR(%d): unable to catch SIGINT\n", errno);

  char * cmds[] = {
    "/bin/bash",
    "-c",
    "cat '/home/arjun/Desktop/file.html'",
    NULL
  };

  int byte_written = 0; char bytes;
  /* Create 2 pipes */
  if(pipe2(I_PIPE_FD, O_CLOEXEC) < 0 || pipe2(O_PIPE_FD, O_CLOEXEC) < 0) /* Creating pipe failed */
    exit(2);

  pid_t pPID = getpid();
  if(VERBOSE) fprintf(stderr, "Parent PID : %d\n", pPID);

  /* Step 6/12 :: Fork a new process */
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
      execvp(cmds[0], cmds);
      break;

      default:
        while(read(STDIN_FILENO, &BYTE, 1))
        {
          /*Step 9/12 :: upon EOF, close pipe, send SIGHUP, restore & exit(0)*/
          if(BYTE == mEOF)
          {
              close(O_PIPE_FD[0]);
              close(O_PIPE_FD[1]);
              kill(-2, SIGHUP);
              exit(0);
          }

          /* Step 7/12 :: Pipe to shell, NO ECHO on PIPE */
          if(BYTE == CR || BYTE == LF)
          {
            byte_written = write(STDOUT_FILENO, &CR, 1);
            BYTE = LF;
          }
          byte_written = write(STDOUT_FILENO, &BYTE, 1);
          byte_written = write(O_PIPE_FD[1], &BYTE, 1);
        }
        break;
    }
  return;
}
static void basic_mode() {
 /* Step 4/12 :: Read char-by-char  */
 /* Step 5/12 :: write char-by-char */
 int byte_written;
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

void sig_handler(int sigN) {
  /* Step 8/12 [part 1/2] :: Send SIGINT to shell */
  if(VERBOSE) fprintf(stderr,"CAUGHT SIGNAL %d, forwarding to child\n", sigN);
  kill(-2, sigN);
}

static void setTC_initial() {
  fprintf(stderr, "ON EXIT:: Set INITIAL mode....");
  if(tcsetattr(STDIN_FILENO, TCSANOW, &TERM_INIT) == 0)
    fprintf(stderr, "SUCCESS\n");
  else
    fprintf(stderr, "FAILED\n");

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
  fprintf(stderr, "ON EXIT:: Set COOKED mode.\n");

  if(tcsetattr(STDIN_FILENO, TCSANOW, &term_cooked) == 0)
    fprintf(stderr, "SUCCESS\n");
  else
    fprintf(stderr, "FAILED\n");

  close(STDIN_FILENO);
  close(STDOUT_FILENO);
}
