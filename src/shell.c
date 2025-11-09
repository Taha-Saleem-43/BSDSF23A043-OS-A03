#include "shell.h"


// Custom completion function
char** my_completion(const char* text, int start, int end) {
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, rl_filename_completion_function);
}

void initialize_readline() {
    rl_attempted_completion_function = my_completion;
    using_history();
}

void reap_background_jobs() {
    int status;
    for (int i = 0; i < jobs_count; ) {
        pid_t ret = waitpid(jobs_list[i].pid, &status, WNOHANG);
        if (ret > 0) {
            // remove job from list
            for (int j = i; j < jobs_count-1; j++)
                jobs_list[j] = jobs_list[j+1];
            jobs_count--;
        } else {
            i++;
        }
    }
}


char** tokenize(char* cmdline) {
    if (cmdline == NULL || cmdline[0] == '\0' || cmdline[0] == '\n') {
        return NULL;
    }

    char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = (char*)malloc(sizeof(char) * ARGLEN);
        bzero(arglist[i], ARGLEN);
    }

    char* cp = cmdline;
    char* start;
    int len;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++;
        if (*cp == '\0') break;

        // Special single-character tokens: <, >, |
        if (*cp == '<' || *cp == '>' || *cp == '|') {
            arglist[argnum][0] = *cp;
            arglist[argnum][1] = '\0';
            cp++;
            argnum++;
            continue;
        }

        // Regular token
        start = cp;
        len = 0;
        while (*cp != '\0' && *cp != ' ' && *cp != '\t' && *cp != '<' && *cp != '>' && *cp != '|') {
            cp++;
            len++;
        }

        if (len > ARGLEN - 1) len = ARGLEN - 1;
        strncpy(arglist[argnum], start, len);
        arglist[argnum][len] = '\0';
        argnum++;
    }

    if (argnum == 0) {
        for (int i = 0; i < MAXARGS + 1; i++) free(arglist[i]);
        free(arglist);
        return NULL;
    }

    arglist[argnum] = NULL;
    return arglist;
}


/* ------------------------------
   NEW FUNCTION FOR BUILT-INS
--------------------------------*/
int handle_builtin(char **arglist) {
    if (arglist == NULL || arglist[0] == NULL)
        return 0;

    // exit command
    if (strcmp(arglist[0], "exit") == 0) {
        printf("Exiting shell...\n");
        exit(0);
    }

    // cd command
    else if (strcmp(arglist[0], "cd") == 0) {
        if (arglist[1] == NULL) {
            fprintf(stderr, "cd: missing argument\n");
        } else {
            if (chdir(arglist[1]) != 0) {
                perror("cd failed");
            }
        }
        return 1;
    }

    // help command
    else if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  cd <dir>   - Change directory\n");
        printf("  help       - Show this help message\n");
        printf("  exit       - Exit the shell\n");
        printf("  jobs       - Placeholder command\n");
        printf("  history    - Show command history\n");
        return 1;
    }

	// jobs command
	else if (strcmp(arglist[0], "jobs") == 0) {
	    reap_background_jobs();  // clean up any finished jobs first
	
	    for (int i = 0; i < jobs_count; i++) {
	        printf("[%d] PID: %d CMD: %s\n", i + 1, jobs_list[i].pid, jobs_list[i].cmd);
	    }
	    return 1;
	}


    // history command
	else if (strcmp(arglist[0], "history") == 0) {
	    int count = history_count < HISTORY_SIZE ? history_count : HISTORY_SIZE;
	
	    for (int i = 0; i < count; i++) {
	        printf("%d %s\n", i + 1, history[i]);
	    }
	    return 1;
	}


    return 0; // Not a built-in
}


