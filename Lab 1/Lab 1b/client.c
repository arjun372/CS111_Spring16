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
#include <error.h>      /* Error handling include headers */
#include <errno.h>
#include <mcrypt.h>
#include <netinet/in.h> /* for htons, and server setup */
#include <netdb.h>
#include <fcntl.h>
#include <pthread.h>

#define TRUE      1
#define FALSE     0
#define N_THREADS 1
#define FILE_MODE 0664
#define KEY_FILE  "my.key"
#define KEY_SIZE  16             /* 128-bit */
#define CIPHER    "twofish"
#define C_MODE    "cfb"

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

/* Encryption Parameters */
static MCRYPT CRYPT; 
static char   O_BUFFER;
static char   I_BUFFER;
static char*  IV;
static char KEY[16] = {0};

/* pthread Worker Pool */
static pthread_t thread_pool[N_THREADS];
static unsigned int validThreads[N_THREADS];
static unsigned long long  R_THREADS = 0;
static unsigned long long  worker_n;

static struct option long_options[] = {
  {"verbose",       no_argument,    &VERBOSE,   1},
  {"encrypt",       no_argument,           0, 'e'},
  {"port",    required_argument,           0, 'p'},
  {"log",     required_argument,           0, 'l'},
};

/* function definitions */
static void setTC_cooked();
static void setTC_initial();
void *doWork_SOCK_2_STDOUT(void *val); /* pThread worker functions */
static void debug_log(const int opt_index, char **optarg, const int argc);

int main(int argc, char **argv) {
  int opt=0, opt_index=0, bytes_read=0, K_FD=-1;
  while(TRUE)
    {
      opt = getopt_long_only(argc, argv, "", long_options, &opt_index);
      
      // all options have been read
      if(opt==-1)
        break;

      switch(opt) {

        case 'e' :
	    if(VERBOSE) debug_log(opt_index, &optarg, 0);
            ENCRYPT = 1;
	    CRYPT = mcrypt_module_open(CIPHER, NULL, C_MODE, NULL);
	    if(CRYPT == MCRYPT_FAILED)
	      goto ENCRYPT_ERR;
            K_FD = open(KEY_FILE, O_RDONLY, FILE_MODE);
	    if(K_FD < 0)
	      goto ENCRYPT_ERR;
	    bytes_read = read(K_FD, &KEY, KEY_SIZE);
	    if(bytes_read != KEY_SIZE)
	      goto ENCRYPT_ERR;
	    IV = malloc(mcrypt_enc_get_iv_size(CRYPT));
	    for(bytes_read = 0; bytes_read < mcrypt_enc_get_iv_size(CRYPT); bytes_read++)
	      IV[bytes_read]=rand();
	    if(mcrypt_generic_init(CRYPT, KEY, KEY_SIZE, IV) < 0)
	      goto ENCRYPT_ERR;
	    break;
      ENCRYPT_ERR:
	    fprintf(stderr, "FATAL: Unable to read 'my.key' file correctly, turning encryption off.\n");
	    ENCRYPT = 0;
	    break;

        case 'p' :
            /*TODO :: Port < 1024 or NaN*/
            PORT = atoi(optarg);
            if(VERBOSE) debug_log(opt_index, &optarg, 1);
            break;

        case 'l' :
            /*TODO: Parse the fileName, Open it and set FD */
            if(VERBOSE) debug_log(opt_index, &optarg, 1);
            LOG_FD = open(optarg, O_WRONLY|O_CREAT, FILE_MODE);
            if(LOG_FD < 0)
              fprintf(stderr, "Unable to open file %s\n", optarg);
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

  if(!PORT)
    {
      fprintf(stderr, "FATAL: Port not specified!\n");
      exit(0);
    }

  /* Step 2/xx :: Open a socket with the server */
  SOCKET_FD = socket(AF_INET, SOCK_STREAM, 0);
  if (SOCKET_FD < 0)
        fprintf(stderr, "FATAL: error opening socket\n");

  /* Connect to the server */
  struct sockaddr_in SRV_ADDR;
  struct hostent *server = gethostbyname("localhost");
  bzero((char *) &SRV_ADDR, sizeof(SRV_ADDR));
  SRV_ADDR.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&SRV_ADDR.sin_addr.s_addr,server->h_length);
  SRV_ADDR.sin_port = htons(PORT);
  if (connect(SOCKET_FD, (struct sockaddr *) &SRV_ADDR,sizeof(SRV_ADDR)) < 0) {
    close(SOCKET_FD);
    exit(1);
    fprintf(stderr, "FATAL: error connecting to host (localhost)\n");
  }
  else if(VERBOSE)
    fprintf(stderr, "SUCCESS: connected to host (localhost)\n");

  /* In a new thread, output data from socket to the display */
  pthread_t socket_thread;
  pthread_create(&socket_thread, NULL, (void *)doWork_SOCK_2_STDOUT, (void *)(NULL));

  /* Read input from keyboard */
  int log_written, byte_written;
  char O_BYTE;
  while(read(STDIN_FILENO, &O_BYTE, 1)) {

    /* ^D entered :: Restore terminal modes & exit with RC = 0*/
    if(O_BYTE == mEOF) 
      {
	close(SOCKET_FD);
	exit(0);
      }
      
    if(O_BYTE == '\r' || O_BYTE == '\n')
      {
        byte_written = write(STDOUT_FILENO, &CR, 1);
        O_BYTE = '\n';
      }
    
    byte_written = write(STDOUT_FILENO, &O_BYTE, 1);

    if(ENCRYPT)
      mcrypt_generic(CRYPT, &O_BYTE, 1);

    byte_written = write(SOCKET_FD, &O_BYTE, 1);
    
    /* Write to socket. If successful && log_on, then write to log_file */
    if(byte_written && (LOG_FD > -1))
      {
        dprintf(LOG_FD, "SENT 1 bytes: ");
        byte_written = write(LOG_FD, &O_BYTE, 1);
        dprintf(LOG_FD, "\n");
      }
  }
  exit(0);
}

void * doWork_SOCK_2_STDOUT(void *val) {
  int byte_written, I_BYTE;
  void * none = val;
  while(read(SOCKET_FD, &I_BYTE, 1))
    {
      if(LOG_FD > -1)
        {
	  dprintf(LOG_FD, "RECEIVED 1 bytes: ");
	  byte_written = write(LOG_FD, &I_BYTE, 1);
	  dprintf(LOG_FD, "\n");
	}

      if(ENCRYPT)
	mdecrypt_generic(CRYPT, &I_BYTE, 1);
	  
      if(I_BYTE == mEOF)
	break;

      byte_written = write(STDOUT_FILENO, &I_BYTE, 1);
   }

  if(VERBOSE) fprintf(stderr, "EOF from socket, exiting (1)\n");
  close(SOCKET_FD);
  exit(1);
  pthread_exit(NULL);
}

static void setTC_initial() {
  if(VERBOSE)fprintf(stderr, "ON EXIT:: Set INITIAL mode....");
  if(tcsetattr(STDIN_FILENO, TCSANOW, &TERM_INIT) == 0)
    {if(VERBOSE)fprintf(stderr, "SUCCESS\n");}
  else
    {if(VERBOSE)fprintf(stderr, "FAILED\n");}

  if(ENCRYPT)
  {
    free(IV);
    mcrypt_generic_deinit(CRYPT);
    mcrypt_module_close(CRYPT);
  }

  if(LOG_FD > -1 ) close(LOG_FD);
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

  if(ENCRYPT)
    {
      free(IV);
      mcrypt_generic_deinit(CRYPT);
      mcrypt_module_close(CRYPT);
    }

  if(LOG_FD > -1 ) close(LOG_FD);
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
