/**
    UCLA CS111 - Spring '16
    Arjun 504078752
    Lab 1b
**/


#include <termios.h>   /* To get & set Terminal attributes */
#include <getopt.h>    /* Argument Options parse headers */
#include <string.h>
#include <stdlib.h>    /* Stdio function headers */
#include <stdio.h>     /* Stdio function headers */
#include <unistd.h>    /* FD numbers */
#include <sys/types.h>
#include <sys/socket.h>
#include <error.h>    /* Error handling include headers */
#include <errno.h>
#include <mcrypt.h>    /* for RIJNDAEL-128 w/ CBC */

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
static int SOCKET_FD = -1;

static struct sockaddr_in SRV_ADDR;

static struct option long_options[] = {
  {"verbose",       no_argument,    &VERBOSE,   1},
  {"encrypt",       no_argument,           0, 'e'},
  {"port",    required_argument,           0, 'p'},
  {"log",     required_argument,           0, 'l'},
};

/* function definitions */
static void setTC_initial();
static void setTC_cooked();
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

        case 'l' :
            /*TODO: Parse the fileName, Open it and set FD */
            if(VERBOSE) debug_log(opt_index, &optarg, 1);
            break;
      }
    }

  if(tcgetattr(STDIN_FILENO, &TERM_INIT) == 0)
    atexit(setTC_initial);
  else {
    if(VERBOSE) fprintf(stderr, "Unable to get tty attr, will set to COOKED on exit.\n");
    atexit(setTC_cooked);
  }

  /* Step 1/13 :: Put console into RAW mode */
  struct termios term_raw = TERM_INIT;
  cfmakeraw(&term_raw);
  term_raw.c_cc[VMIN]  = 1;
  term_raw.c_cc[VTIME] = 0;
  term_raw.c_lflag |= ISIG;

  if(tcsetattr(STDIN_FILENO, TCSANOW, &term_raw) != 0)
    fprintf(stderr, "FATAL :: Unable to switch to RAW mode, no guanrantees from this point on!\n");

  /* Step 2/xx :: Open a socket with the server */
  SOCKET_FD = socket(AF_INET, SOCK_STREAM, 0);
  if (SOCKET_FD < 0)
        fprintf(stderr, "FATAL: error opening socket\n");

  /* SRV_ADDR = */
  while(read(STDIN_FILENO, &BYTE, 1)) {
    /* ^D entered */
    if(BYTE == mEOF) {
      // TODO 1 : close network connection
      // Restore terminal modes & exit with RC = 0
      exit(0);
    }

    if(BYTE == CR || BYTE == LF) {
        byte_written = write(STDOUT_FILENO, &CR, 1);
        BYTE = LF;
    }

    byte_written = write(STDOUT_FILENO, &BYTE, 1);
  }

  exit(0);
}

static void setTC_initial() {
  if(VERBOSE)fprintf(stderr, "ON EXIT:: Set INITIAL mode....");
  if(tcsetattr(STDIN_FILENO, TCSANOW, &TERM_INIT) == 0)
    {if(VERBOSE)fprintf(stderr, "SUCCESS\n");}
  else
    {if(VERBOSE)fprintf(stderr, "FAILED\n");}

  /*if(SHELL)
  {
    int child_status = 8888;
    waitpid(CHILD_PID, &child_status, 0);
    printf("Child exited with status : %d\n", WEXITSTATUS(child_status));
  }*/
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

  /*  if(SHELL)
    {
      int child_status = 8888;
      waitpid(CHILD_PID, &child_status, 0);
      printf("Child exited with status : %d\n", WEXITSTATUS(child_status));
    }*/
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
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
