// UCLA CS 111 Lab 1 command interface
#ifndef COMMAND_H
#define COMMAND_H
#endif
typedef struct command *command_t;
typedef struct command_stream *command_stream_t;

/* Print a command to stdout, for debugging.  */
void print_command (int flags, int opt_index, char **argv, int cmd_args);

/* Execute a command.  Use "time travel" if the integer flag is
   nonzero.  */
//void execute_command (int flags, int *fid_arr, int opt_index, char **argv, int cmd_args);

/* Return the exit status of a command, which must have previously been executed.
   Wait for the command, if it is not already finished.  */
int command_status (command_t);
