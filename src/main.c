#include "shell.h"


char* history[HISTORY_SIZE];
int history_count = 0;  // total number of commands entered


void add_to_history(const char* cmdline) {
    if (history_count < HISTORY_SIZE) {
        history[history_count] = strdup(cmdline);
    } else {
        // History full: remove oldest command and shift everything left
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++) {
            history[i-1] = history[i];
        }
        history[HISTORY_SIZE - 1] = strdup(cmdline);
    }
    history_count++;
}



int handle_bang_command(char** cmdline_ptr) {
    char* cmdline = *cmdline_ptr;

    // Check if command starts with '!'
    if (cmdline[0] == '!') {
        // Parse the number after '!'
        int n = 0;
        for (int i = 1; cmdline[i] != '\0'; i++) {
            if (!isdigit(cmdline[i])) {
                fprintf(stderr, "Invalid !n command\n");
                return 0;
            }
            n = n * 10 + (cmdline[i] - '0');
        }

        // n must be between 1 and history_count
        if (n < 1 || n > history_count) {
            fprintf(stderr, "No such command in history: !%d\n", n);
            return 0;
        }

        // Replace cmdline with the retrieved command
        free(*cmdline_ptr);  // free old !n string
        *cmdline_ptr = strdup(history[n - 1]);
        printf("%s\n", *cmdline_ptr); // Optional: echo the command being executed
        return 1;
    }

    return 0; // Not a !n command
}




int main() {
    char* cmdline;
    char** arglist;
    
    initialize_readline();


	while ((cmdline = readline(PROMPT)) != NULL) {
	
	    if (*cmdline == '\0') {
	        free(cmdline);
	        continue; // skip empty input
	    }
	
	    handle_bang_command(&cmdline);    // handle !n
	    add_to_history(cmdline);           // numbered history
	
	    add_history(cmdline);              // GNU Readline history
	
	    if ((arglist = tokenize(cmdline)) != NULL) {
	        if (!handle_builtin(arglist)) {
	            execute(arglist);
	        }
	
	        for (int i = 0; arglist[i] != NULL; i++)
	            free(arglist[i]);
	        free(arglist);
	    }
	
	    free(cmdline);
	}



    printf("\nShell exited.\n");
    return 0;
}

