#ifndef SHELL_H
#define SHELL_H
#define MAX_JOBS 100
#define MAX_BLOCK_LINES 50
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "FCIT> "
#define HISTORY_SIZE 20

// History support
extern char* history[HISTORY_SIZE];
extern int history_count;

// Feature 7: if-then-else-fi support
typedef struct {
    char* then_block[MAX_BLOCK_LINES];
    char* else_block[MAX_BLOCK_LINES];
    int then_count;
    int else_count;
    char* condition_cmd;
} if_block_t;

// Background jobs (Feature 6)
typedef struct {
    pid_t pid;
    char cmd[256];
} job_t;

extern job_t jobs_list[MAX_JOBS];
extern int jobs_count;

// Function prototypes from shell.c
char** tokenize(char* cmdline);
int handle_builtin(char** args);
void initialize_readline();
void reap_background_jobs();
char** my_completion(const char* text, int start, int end);

// Function prototypes from main.c
int execute(char** arglist);
void add_to_history(const char* cmdline);
int handle_bang_command(char** cmdline_ptr);

// Function prototypes from execute.c
int execute(char** arglist);

// Feature 7: if-then-else-fi functions (main.c)
int is_if_statement(const char* cmd);
int is_keyword(const char* line, const char* keyword);
void trim_string(char* str);
int read_if_block(if_block_t* block);
int execute_condition(const char* cmd);
void execute_block(char** block, int count);
int execute_if_block(if_block_t* block);
int handle_if_statement(char* cmdline);

#endif // SHELL_H
