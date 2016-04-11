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
static char BYTE;
static char CR   = 0x0D;
static char LF   = 0x0A;
static char mEOF = 0x04;

static int VERBOSE = 0;
static char *ptr = NULL;

static struct option long_options[] = {
  {"shell", no_argument,          0, 's'},
  {"verbose", no_argument, &VERBOSE,   1},
};

/* function definitions */
static void debug_log(const int opt_index, char **optarg, const int argc);
static void setTC_initial();
static void setTC_cooked();
void sig_handler(int sigN);

/* To be run at exit */
int main(int argc, char **argv) {

 int ARGC = argc; char **ARGV = argv;

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


 /* Step 4/12 :: Read char-by-char  */
 /* Step 5/12 :: write char-by-char */
 int byte_written;
 while(read(STDIN_FILENO, &BYTE, 1))
 {
   if(BYTE == 0x04)
     break;

   if(BYTE == CR || BYTE == LF) {
     byte_written = write(STDOUT_FILENO, &CR, 1);
     BYTE = LF;
   }

   byte_written = write(STDOUT_FILENO, &BYTE, 1);
 }

  exit(0);
}

void sig_handler(int sigN) {
  fprintf(stderr, "Caught SIGNAL %d\n", sigN);
  exit(CATC_ERR);
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

/* Logs to stdout */
static void debug_log(const int opt_index, char **optarg, const int argc)
{
  if(!VERBOSE)
    return;

  int i;
  fprintf(stderr,"--%s", long_options[opt_index].name);
  for(i = 0; i < argc; i++)
    fprintf(stderr," %s", optarg[i]);
  fprintf(stderr,"\n");
}
