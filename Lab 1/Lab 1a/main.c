/**
    UCLA CS111 - Spring '16
    Arjun 504078752
    Lab 1a
**/

#define TRUE  1

#define OPEN_ERR 1
#define CREA_ERR 2
#define CATC_ERR 3

#define FILE_MODE 0664

#define MAX_LONG_OPTIONS 5

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


static char BYTE;
static int VERBOSE = 0;
static char *ptr = NULL;

static struct option long_options[] = {
  {"input",    required_argument,  0, 'r'},
  {"output",   required_argument,  0, 'w'},
  {"segfault", no_argument,        0, 's'},
  {"catch",    no_argument,        0, 'c'},
  {"verbose",  no_argument, &VERBOSE,   1},
};

/* function definitions */
static int  isOption(const char *opt);
static void debug_log(const int opt_index, char **optarg, const int argc);

void sig_handler(int sigN) {
  fprintf(stderr, "Caught SIGNAL %d\n", sigN);
  exit(CATC_ERR);
}

int main(int argc, char **argv) {
  int fopen_flag, opt=0, opt_index=0;
  ssize_t noUse;

  while(TRUE)
    {
      opt = getopt_long_only(argc, argv, "", long_options, &opt_index);

      // all options have been read
      if(opt==-1)
        break;

      switch(opt)
      {
        case 'r':    /* --input  arg1 */
        case 'w':    /* --output arg1 */

          if(isOption(optarg))
          /* error check 1 -- argument is an option (ambigous fname) */
          {
            fprintf(stderr, "--%s %s :: argument '%s' is an option!\n", long_options[opt_index].name, optarg, optarg);
            goto fopen_err;
          }

        case 's':    /* --segfault    */


          if(opt=='s') /* create segmentation fault */
            {
	      *ptr = opt;//raise(SIGSEGV);
              continue;
            }


          if(opt=='r' && (close(STDIN_FILENO)==0) && (open(optarg, O_RDONLY, FILE_MODE) >= 0))
            continue;

          if(opt=='w' && (close(STDOUT_FILENO)==0) && (open(optarg, O_WRONLY|O_CREAT, FILE_MODE) >= 0))
            continue;

          /* Unable to open file, show error */
          perror(optarg);
          fopen_err:
          fprintf(stderr, "--%s %s :: %d\n", long_options[opt_index].name, optarg, errno);
          exit(opt=='r' ? OPEN_ERR : (opt=='w' ? CREA_ERR : CATC_ERR));

          case 'c':    /* --catch  sigN */
          debug_log(opt_index, &optarg, 1);
          if(signal(SIGSEGV, sig_handler) == SIG_ERR)
            goto fopen_err;
          break;
      }
    }

  while(read(STDIN_FILENO, &BYTE, 1))
    noUse = write(STDOUT_FILENO, &BYTE, 1);

  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  
  exit(0);
}

/* Checks if the given string is a valid option as defined by the spec.    *
 * Helps in checking whether arguments passed to options are valid or not. */
static int isOption(const char *opt)
{
  int i;
  for(i=0; i<(MAX_LONG_OPTIONS-1); i++)
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
