/** 
    UCLA CS111 - Spring '16
    Arjun 504078752
    Lab 0
**/

#include <getopt.h>
#include <error.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define TRUE  1
#define FALSE 0
#define I_FD  0
#define O_FD  1
#define FILE_MODE 0664

static int FID[2] = {0};
static int exit_status = 0;
static struct option long_options[] = {
  {"catch",    no_argument, 0, 'c'},
  {"input",    required_argument, 0, 'r'},
  {"output",   required_argument, 0, 'w'},
  {"segfault", no_argument, 0, 's'},
};

int main(int argc, char **argv)
{
  int fopen_flag, opt=0, opt_index=0;

  while(TRUE)
    {
      opt = getopt_long_only(argc, argv, "", long_options, &opt_index);
    
      // all options have been read
      if(opt==-1)
	break;

      switch(opt) 
	{
	case 'r':              /* --input arg1 */
	  if(isOption(optarg))
	    goto fopen_err;

	  if(DEBUG)
	    log(opt_index, &optarg, 1);
	  
	  if((FID[I_FD] = open(optarg, O_RDONLY, FILE_MODE)) >= 0)
	    break;
	  
	fopen_err:
	  exit_status
   
	case 'w':              /* --output arg1 */
	  fopen_flag=O_WRONLY;
	fopen_exec:
	  if(isOption(optarg))
	    goto fopen_err;
	  

