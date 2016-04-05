/**
    UCLA CS111 - Spring '16
    Arjun 504078752
    Lab 0
**/
#define DEBUG 1       /* for verbose output */
#define BUF_SIZE 4096

#define TRUE  1
#define FALSE 0

#define OPEN_ERR 1
#define CREA_ERR 2
#define CATC_ERR 3

#define I_FD  0
#define O_FD  1
#define FILE_MODE 0664

#define MAX_LONG_OPTIONS 4

#include <getopt.h>
#include <error.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* open(2) include headers */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static int FID[2] = {-1,-1}; /* indicates that fd will be std, stdout */
static int exit_status = 0;
static char buffer[BUF_SIZE] = {0};

static struct option long_options[] = {
  {"catch",    no_argument, 0, 'c'},
  {"input",    required_argument, 0, 'r'},
  {"output",   required_argument, 0, 'w'},
  {"segfault", no_argument, 0, 's'},
};

/* function definitions */
static int isOption(const char *opt);
static pid_t execute_command (const size_t proc_index);
static void debug_log(const int opt_index, char **optarg, const int argc);
static int usage (const int errNum, const char *opt_name, const char *optarg);

int main(int argc, char **argv) {
  int fopen_flag, opt=0, opt_index=0;

  while(TRUE)
    {
      opt = getopt_long_only(argc, argv, "", long_options, &opt_index);

      // all options have been read
      if(opt==-1)
        break;

      switch(opt)
      {
        case 'r':              /* --input  arg1 */
        case 'w':              /* --output arg1 */
        case 'c':

          if(isOption(optarg))
          /* error check 1 -- argument is an option (ambigous fname) */
          {
            fprintf(stderr, "--%s %s :: argument '%s' is an option!\n", long_options[opt_index].name, optarg, optarg);
            goto fopen_err;
          }

          if(DEBUG)
            debug_log(opt_index, &optarg, 1);

          if(opt=='r' && ((FID[I_FD] = open(optarg, O_RDONLY, FILE_MODE)) >= 0))
            continue;

          if(opt=='w' && ((FID[O_FD] = open(optarg, O_WRONLY|O_CREAT, FILE_MODE)) >= 0))
            continue;

          /* Unable to open file, show error */
          perror(optarg);
          fopen_err:
          fprintf(stderr, "--%s %s :: %d\n", long_options[opt_index].name, optarg, errno);
          exit(opt=='r'? OPEN_ERR : (opt=='w' ? CREA_ERR : CATC_ERR));
      }
    }
return 0;
}

/* Checks if the givent string is a valid option as defined by the spec.    *
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
  int i;
  printf("--%s", long_options[opt_index].name);
  for(i = 0; i < argc; i++)
    printf(" %s", optarg[i]);
  printf("\n");
}
