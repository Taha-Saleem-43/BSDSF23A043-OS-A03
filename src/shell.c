/* shell.c
 * Contains: Utility functions, built-in command handler, Feature 8 (variables)
 * Features: Readline setup, background job reaping, tokenization, built-ins, variables
 * Called by: main.c
 * Feature 8 functions: Variable storage, expansion, and display
 */

#include "shell.h"

// Feature 8: Global variables linked list head
var_node_t* variables_head = NULL;

// ============ READLINE FUNCTIONS (Feature 4) ============

char** my_completion(const char* text, int start, int end) {
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, rl_filename_completion_function);
}

void initialize_readline() {
    rl_attempted_completion_function = my_completion;
    using_history();
}

// ============ BACKGROUND JOBS (Feature 6) ============

void reap_background_jobs() {
    int status;
    for (int i = 0; i < jobs_count; ) {
        pid_t ret = waitpid(jobs_list[i].pid, &status, WNOHANG);
        if (ret > 0) {
            for (int j = i; j < jobs_count-1; j++)
                jobs_list[j] = jobs_list[j+1];
            jobs_count--;
        } else {
            i++;
        }
    }
}

// ============ TOKENIZE (Feature 1) ============

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
        
        if (*cp == '<' || *cp == '>' || *cp == '|') {
            arglist[argnum][0] = *cp;
            arglist[argnum][1] = '\0';
            cp++;
            argnum++;
            continue;
        }
        
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

// ============ FEATURE 8: SHELL VARIABLES ============

// Find variable by name
var_node_t* find_variable(const char* name) {
    var_node_t* current = variables_head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Set or create variable
void set_variable(const char* name, const char* value) {
    if (name == NULL || value == NULL) return;
    
    // Check if variable already exists
    var_node_t* existing = find_variable(name);
    if (existing != NULL) {
        free(existing->value);
        existing->value = strdup(value);
        return;
    }
    
    // Create new variable node
    var_node_t* new_node = (var_node_t*)malloc(sizeof(var_node_t));
    if (new_node == NULL) return;
    
    new_node->name = strdup(name);
    new_node->value = strdup(value);
    new_node->next = variables_head;
    variables_head = new_node;
}

// Print all variables
void print_all_variables() {
    var_node_t* current = variables_head;
    
    if (current == NULL) {
        printf("No variables set\n");
        return;
    }
    
    printf("Shell Variables:\n");
    while (current != NULL) {
        printf("%s=%s\n", current->name, current->value);
        current = current->next;
    }
}

// Free all variables
void free_all_variables() {
    var_node_t* current = variables_head;
    while (current != NULL) {
        var_node_t* next = current->next;
        free(current->name);
        free(current->value);
        free(current);
        current = next;
    }
    variables_head = NULL;
}

// Check if command is variable assignment
int is_assignment(const char* cmd) {
    if (cmd == NULL) return 0;
    
    const char* equal_sign = strchr(cmd, '=');
    if (equal_sign == NULL) return 0;
    
    // Check if there are spaces around =
    if (equal_sign == cmd || equal_sign[1] == '\0') return 0;
    
    // Check if name part is valid (no spaces before =)
    for (const char* p = cmd; p < equal_sign; p++) {
        if (*p == ' ' || *p == '\t') return 0;
    }
    
    return 1;
}

// Expand variables in argument list
char** expand_variables(char** arglist) {
    if (arglist == NULL) return NULL;
    
    // Count arguments
    int count = 0;
    while (arglist[count] != NULL) count++;
    
    // Create new expanded argument list
    char** expanded = (char**)malloc(sizeof(char*) * (count + 1));
    
    for (int i = 0; i < count; i++) {
        if (arglist[i][0] == '$') {
            // Variable expansion
            const char* var_name = &arglist[i][1];
            var_node_t* var = find_variable(var_name);
            
            if (var != NULL) {
                expanded[i] = (char*)malloc(strlen(var->value) + 1);
                strcpy(expanded[i], var->value);
            } else {
                // Variable not found, keep original (empty or $NAME)
                expanded[i] = (char*)malloc(strlen(arglist[i]) + 1);
                strcpy(expanded[i], "");
            }
        } else {
            // No variable expansion needed
            expanded[i] = (char*)malloc(strlen(arglist[i]) + 1);
            strcpy(expanded[i], arglist[i]);
        }
    }
    
    expanded[count] = NULL;
    
    // Free old arglist
    for (int i = 0; i < count; i++) {
        free(arglist[i]);
    }
    free(arglist);
    
    return expanded;
}

// ============ BUILT-IN HANDLER (Features 1, 6, 8) ============

int handle_builtin(char **arglist) {
    if (arglist == NULL || arglist[0] == NULL)
        return 0;
    
    // exit command
    if (strcmp(arglist[0], "exit") == 0) {
        printf("Exiting shell...\n");
        free_all_variables();
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
    // Help command after the issue is resolved
    else if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  cd <dir>            - Change directory\n");
        printf("  help                - Show this help message\n");
        printf("  exit                - Exit the shell\n");
        printf("  jobs                - List background jobs\n");
        printf("  history             - Show command history\n");
        printf("  set                 - Show all variables\n");
        return 1;
    }
    // jobs command (Feature 6)
    else if (strcmp(arglist[0], "jobs") == 0) {
        reap_background_jobs();
        
        for (int i = 0; i < jobs_count; i++) {
            printf("[%d] PID: %d CMD: %s\n", i + 1, jobs_list[i].pid, jobs_list[i].cmd);
        }
        return 1;
    }
    // history command (Feature 4)
    else if (strcmp(arglist[0], "history") == 0) {
        int count = history_count < HISTORY_SIZE ? history_count : HISTORY_SIZE;
        
        for (int i = 0; i < count; i++) {
            printf("%d %s\n", i + 1, history[i]);
        }
        return 1;
    }
    // set command (Feature 8)
    else if (strcmp(arglist[0], "set") == 0) {
        print_all_variables();
        return 1;
    }
    
    return 0; // Not a built-in
}
