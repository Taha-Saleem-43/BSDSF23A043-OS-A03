#ifndef SHELL_H
#define SHELL_H
#define MAX_JOBS 100



#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>  // for isdigit()
#include <errno.h>

//Additional External Libraries
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "FCIT> "

// --------- History support ---------
#define HISTORY_SIZE 20
extern char* history[HISTORY_SIZE];  // shared array for last commands
extern int history_count;            // number of commands stored

// Function prototypes
char* read_cmd(char* prompt, FILE* fp);
char** tokenize(char* cmdline);
int execute(char** arglist);
int handle_builtin(char** args);

// History-related functions
void add_to_history(const char* cmdline);
int handle_bang_command(char** cmdline_ptr);




// Readline initialization
void initialize_readline();

//feature 6
typedef struct {
    pid_t pid;
    char cmd[256];
} job_t;

//feature 6
void reap_background_jobs();


// Declare (not define) here
extern job_t jobs_list[MAX_JOBS];
extern int jobs_count;

#endif // SHELL_H

