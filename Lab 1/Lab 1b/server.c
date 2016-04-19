/**
    UCLA CS111 - Spring '16
    Arjun 504078752
    Lab 1b
**/

#define _GNU_SOURCE
#include <termios.h>   /* To get & set Terminal attributes */
#include <getopt.h>    /* Argument Options parse headers */
#include <string.h>
#include <stdlib.h>    /* Stdio function headers */
#include <stdio.h>     /* Stdio function headers */
#include <unistd.h>    /* FD numbers */
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <error.h>      /* Error handling include headers */
#include <errno.h>
#include <mcrypt.h>     /* for RIJNDAEL-128 w/ CBC */
#include <netinet/in.h> /* for htons, and server setup */
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#define TRUE      1
#define FALSE     0

static struct termios TERM_INIT;
static char CR      = 0x0D;
static char LF      = 0x0A;
static char mEOF    = 0x04;
static char mSIGINT = 0x03;

static int VERBOSE   =  0;
static int PORT      =  0;
static int LOG_FD    = -1;
static int ENCRYPT   =  0;
static int SOCKET_FD = -1, newSOCKET_FD;

static pid_t CHILD_PID = -2;
static int I_PIPE_FD[2] = {-1, -1};
static int O_PIPE_FD[2] = {-1, -1};

static struct option long_options[] = {
  {"verbose",       no_argument,    &VERBOSE,   1},
  {"encrypt",       no_argument,           0, 'e'},
  {"port",    required_argument,           0, 'p'},
};

/* function definitions */
static void child_status();
void *doWork_SHELL_2_STDOUT(void *val);
static void debug_log(const int opt_index, char **optarg, const int argc);

int main(int argc, char **argv) {
  int opt=0, opt_index=0, byte_written, BYTE;
  while(TRUE)
    {
      opt = getopt_long_only(argc, argv, "", long_options, &opt_index);
      // all options have been read
      if(opt==-1)
        break;

      switch(opt) {

        case 'e' :
            ENCRYPT = 1;
            //TODO :: more encryption stuff
            if(VERBOSE) debug_log(opt_index, &optarg, 0);
            break;

        case 'p' :
            /*TODO :: Port < 1024 or NaN*/
            PORT = atoi(optarg);
            if(VERBOSE) debug_log(opt_index, &optarg, 1);
            break;
      }
    }

  struct sockaddr_in SRV_ADDR, CLI_ADDR;
  socklen_t CLI_LEN;
  bzero((char *) &SRV_ADDR, sizeof(SRV_ADDR));
  SRV_ADDR.sin_family = AF_INET;
  SRV_ADDR.sin_addr.s_addr = INADDR_ANY;
  SRV_ADDR.sin_port = htons(PORT);

  /* Step 1/xx :: Open a socket */
  SOCKET_FD = socket(AF_INET, SOCK_STREAM, 0);
  if (SOCKET_FD < 0) {
        fprintf(stderr, "FATAL: error opening socket\n");
        exit(1);
      }

  if (bind(SOCKET_FD, (struct sockaddr *) &SRV_ADDR, sizeof(SRV_ADDR)) < 0) {
    fprintf(stderr, "ERROR on binding to socket on port.\n");
    exit(1);
  }
  /* Listen, then accept onConnect */
  listen(SOCKET_FD, 5);
  CLI_LEN = sizeof(CLI_ADDR);
  newSOCKET_FD = accept(SOCKET_FD, (struct sockaddr *) &CLI_ADDR, &CLI_LEN);
  if (newSOCKET_FD < 0) {
    fprintf(stderr,"FATAL: ERROR on accept connection from client\n");
    exit(1);
  }
  else if (VERBOSE)
      fprintf(stderr, "Accepted Connection from client\n");


  char * cmds[] = {"/bin/bash", NULL};
  /* Create 2 pipes */
  if(pipe2(I_PIPE_FD, 0) < 0 || pipe2(O_PIPE_FD, 0) < 0) /* Creating pipe failed */
    exit(2);

  /* In a new thread, output data from socket to the display */
  pthread_t socket_thread;
  pthread_create(&socket_thread, NULL, (void *)doWork_SHELL_2_STDOUT, (void *)(NULL));

  pid_t pPID = getpid();
  if(VERBOSE) fprintf(stderr, "Parent PID : %d\n", pPID);
  /* Step 4/13 :: Fork a new process */
  switch(pPID = fork())
    {
      case -1 : /* fork() failed */
      fprintf(stderr, "FATAL: error forking child process\n");
      close(newSOCKET_FD);
      close(SOCKET_FD);
      exit(2);
      break;

      case 0 : /* In child's (shell's) process */
      if(VERBOSE) fprintf(stderr, "Child PID : %d\n", getpid());
      dup2(O_PIPE_FD[0], STDIN_FILENO);
      dup2(I_PIPE_FD[1], STDOUT_FILENO);
      dup2(I_PIPE_FD[1], STDERR_FILENO);
      close(I_PIPE_FD[0]);
      execvp(cmds[0], cmds);
      break;

      default:
        atexit(child_status);
        CHILD_PID = pPID;
        close(I_PIPE_FD[1]);
        /* Redirect server process STDIN, STDOUT, STDERR to SOCKET_FD */
        dup2(newSOCKET_FD, STDIN_FILENO);
        dup2(newSOCKET_FD, STDOUT_FILENO);
        dup2(newSOCKET_FD, STDERR_FILENO);
        /* Step 2/13 :: Read char-by-char   */
        /* Step 3/13 :: Handles long inputs */
        /* Step 5/13 :: write char-by-char  */
        if(VERBOSE) dprintf(STDERR_FILENO, "IN PARENT STDERR\n");
        if(VERBOSE) dprintf(STDOUT_FILENO, "IN PARENT STDOUT\n");
        while(read(STDIN_FILENO, &BYTE, 1))
        {
            if(BYTE == mSIGINT)
            {
              if(VERBOSE) fprintf(stderr,"CAUGHT SIGNAL %d, forwarding to CHILD %d\n", mSIGINT, CHILD_PID);
              kill(CHILD_PID, SIGINT);
            }

            /* Step 9/12 :: upon EOF, close pipe, send SIGHUP, restore & exit(0) */
            if(BYTE == mEOF)
            {
                if(VERBOSE) fprintf(stderr, "EOF or read error from network client, exiting (1)\n");
                close(O_PIPE_FD[0]);
                close(O_PIPE_FD[1]);
                kill(CHILD_PID, SIGHUP);
                exit(1);
            }

            /* Step 7/12 :: Pipe to shell, NO ECHO on PIPE */
            if(BYTE == '\r')
            {
              byte_written = write(O_PIPE_FD[1], &CR, 1);
              BYTE = LF;
          } //else
          //  byte_written = write(STDOUT_FILENO, &BYTE, 1);
            byte_written = write(O_PIPE_FD[1], &BYTE, 1);
        }
        /* read error or EOF on network connection */
        if(VERBOSE) fprintf(stderr, "EOF or read error from network client, exiting (1)\n");
        close(newSOCKET_FD);
        close(SOCKET_FD);
        kill(CHILD_PID, SIGHUP);
        exit(1);
    }

  exit(0);
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

    /* Server gets EOF or SIGPIPE from shell */
    if(VERBOSE)fprintf(stderr, "EOF or SIGPIPE from shell, exiting (2)\n");
    close(newSOCKET_FD);
    close(SOCKET_FD);
    kill(CHILD_PID, SIGHUP);
    exit(2);

  pthread_exit(NULL);
}

static void child_status() {
  int child_status = 8888;
  waitpid(CHILD_PID, &child_status, 0);
  if(VERBOSE) fprintf(stderr, "Child exited with status : %d\n", WEXITSTATUS(child_status));
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
}

/* Logs to stdout */
static void debug_log(const int opt_index, char **optarg, const int argc) {
  if(!VERBOSE)
    return;

  int i;
  fprintf(stderr,"--%s", long_options[opt_index].name);
  for(i = 0; i < argc; i++)
    fprintf(stderr," %s", optarg[i]);
  fprintf(stderr,"\n");
}
