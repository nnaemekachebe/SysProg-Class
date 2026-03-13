#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include "dshlib.h"

// Global variable to store last return code (for extra credit)
static int last_return_code = 0;

//===================================================================
// HELPER FUNCTIONS - Memory Management (PROVIDED)
//===================================================================

/**
 * alloc_cmd_buff - Allocate memory for cmd_buff internal buffer
 */
int alloc_cmd_buff(cmd_buff_t *cmd_buff)
{
    cmd_buff->_cmd_buffer = malloc(SH_CMD_MAX);
    if (cmd_buff->_cmd_buffer == NULL) {
        return ERR_MEMORY;
    }
    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    return OK;
}

/**
 * free_cmd_buff - Free cmd_buff internal buffer
 */
int free_cmd_buff(cmd_buff_t *cmd_buff)
{
    if (cmd_buff->_cmd_buffer != NULL) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    cmd_buff->argc = 0;
    return OK;
}

/**
 * clear_cmd_buff - Reset cmd_buff without freeing memory
 */
int clear_cmd_buff(cmd_buff_t *cmd_buff)
{
    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    return OK;
}

//===================================================================
// PARSING FUNCTIONS - From Part 1
//===================================================================

/**
 * Helper function to trim leading and trailing whitespace
 */
static char *trim_whitespace(char *str)
{
    while (isspace((unsigned char)*str)) {
        str++;
    }
    
    if (*str == '\0') {
        return str;
    }
    
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }
    
    *(end + 1) = '\0';
    return str;
}

/**
 * build_cmd_buff - Parse a single command string into cmd_buff_t
 * 
 * This function takes a single command string (no pipes) and parses
 * it into argc/argv format. It handles quoted strings to preserve
 * spaces within them.
 */
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff)
{
    // Check for NULL inputs
    if (cmd_line == NULL || cmd_buff == NULL) {
        return ERR_MEMORY;
    }
    
    // Trim leading/trailing whitespace from input
    char *trimmed = trim_whitespace(cmd_line);
    
    // Check if empty after trimming
    if (*trimmed == '\0') {
        cmd_buff->argc = 0;
        return OK;
    }
    
    // Copy the trimmed string to the internal buffer
    strncpy(cmd_buff->_cmd_buffer, trimmed, SH_CMD_MAX - 1);
    cmd_buff->_cmd_buffer[SH_CMD_MAX - 1] = '\0';
    
    // Parse the command line into tokens with quote handling
    int argc = 0;
    char *buffer = cmd_buff->_cmd_buffer;
    int input_len = strlen(buffer);
    int pos = 0;
    
    while (argc < CMD_ARGV_MAX - 1 && pos < input_len) {
        // Skip whitespace
        while (pos < input_len && (buffer[pos] == ' ' || buffer[pos] == '\t')) {
            pos++;
        }
        
        if (pos >= input_len) {
            break;
        }
        
        // Check for quoted strings
        char *arg_start;
        
        if (buffer[pos] == '"') {
            // Double-quoted string
            pos++; // Skip opening quote
            arg_start = &buffer[pos];
            
            // Find closing quote
            while (pos < input_len && buffer[pos] != '"') {
                pos++;
            }
            
            // Null-terminate at closing quote
            if (pos < input_len) {
                buffer[pos] = '\0';
                pos++; // Skip closing quote
            }
        } else if (buffer[pos] == '\'') {
            // Single-quoted string
            pos++; // Skip opening quote
            arg_start = &buffer[pos];
            
            // Find closing quote
            while (pos < input_len && buffer[pos] != '\'') {
                pos++;
            }
            
            // Null-terminate at closing quote
            if (pos < input_len) {
                buffer[pos] = '\0';
                pos++; // Skip closing quote
            }
        } else {
            // Regular argument - find end
            arg_start = &buffer[pos];
            while (pos < input_len && buffer[pos] != ' ' && buffer[pos] != '\t') {
                pos++;
            }
            
            // Null-terminate at end of argument
            if (pos < input_len) {
                buffer[pos] = '\0';
                pos++;
            }
        }
        
        // Store the argument
        cmd_buff->argv[argc] = arg_start;
        argc++;
    }
    
    // Set argc and NULL terminate the argv array
    cmd_buff->argc = argc;
    cmd_buff->argv[argc] = NULL;
    
    return OK;
}

//===================================================================
// BUILT-IN COMMAND FUNCTIONS
//===================================================================

/**
 * match_command - Check if input is a built-in command
 */
Built_In_Cmds match_command(const char *input)
{
    if (input == NULL) {
        return BI_NOT_BI;
    }
    if (strcmp(input, EXIT_CMD) == 0) {
        return BI_CMD_EXIT;
    }
    if (strcmp(input, "cd") == 0) {
        return BI_CMD_CD;
    }
    if (strcmp(input, "rc") == 0) {
        return BI_CMD_RC;
    }
    return BI_NOT_BI;
}

/**
 * exec_built_in_cmd - Execute built-in commands
 * Returns:
 *   BI_CMD_EXIT - exit command was executed
 *   BI_EXECUTED - built-in command was executed successfully
 *   BI_NOT_BI - not a built-in command
 */
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd)
{
    if (cmd == NULL || cmd->argv[0] == NULL) {
        return BI_NOT_BI;
    }
    
    Built_In_Cmds bi_cmd = match_command(cmd->argv[0]);
    
    switch (bi_cmd) {
        case BI_CMD_EXIT:
            // Exit is handled in main loop
            return BI_CMD_EXIT;
            
        case BI_CMD_CD:
            // CD built-in: change directory
            if (cmd->argc < 2) {
                // No argument - do nothing (stay in current directory)
                return BI_EXECUTED;
            }
            if (chdir(cmd->argv[1]) != 0) {
                perror("cd");
            }
            return BI_EXECUTED;
            
        case BI_CMD_RC:
            // Extra credit: print last return code
            printf("%d\n", last_return_code);
            return BI_EXECUTED;
            
        default:
            return BI_NOT_BI;
    }
}

//===================================================================
// EXECUTION FUNCTIONS - Fork/Exec Pattern
//===================================================================

/**
 * exec_cmd - Execute an external command using fork/exec
 * 
 * This function:
 *   1. Forks the process
 *   2. Child executes the command using execvp()
 *   3. Parent waits for child using waitpid()
 *   4. Returns the exit code of the command
 */
int exec_cmd(cmd_buff_t *cmd)
{
    if (cmd == NULL || cmd->argv[0] == NULL) {
        return ERR_EXEC_CMD;
    }
    
    // Fork the process
    pid_t pid = fork();
    
    // Check fork result
    if (pid < 0) {
        // Fork failed
        perror("fork");
        return ERR_EXEC_CMD;
    }
    else if (pid == 0) {
        // Child process - execute command
        execvp(cmd->argv[0], cmd->argv);
        
        // If execvp returns, it failed
        // Print error message based on errno
        if (errno == ENOENT) {
            fprintf(stderr, "Command not found in PATH\n");
            exit(127); // Standard shell exit code for command not found
        }
        else if (errno == EACCES) {
            fprintf(stderr, "Permission denied\n");
            exit(126); // Standard shell exit code for permission denied
        }
        else {
            perror("execvp");
            exit(ERR_EXEC_CMD);
        }
    }
    else {
        // Parent process - wait for child
        int status;
        waitpid(pid, &status, 0);
        
        // Extract exit code
        if (WIFEXITED(status)) {
            last_return_code = WEXITSTATUS(status);
            return last_return_code;
        }
        return ERR_EXEC_CMD;
    }
}

//===================================================================
// MAIN SHELL LOOP
//===================================================================

/**
 * exec_local_cmd_loop - Main shell loop
 * 
 * This function:
 *   1. Loop forever (while(1))
 *   2. Print the shell prompt (SH_PROMPT)
 *   3. Read a line of input using fgets()
 *   4. Remove trailing newline
 *   5. Check for exit command - if found, print "exiting..." and break
 *   6. Parse the command line using build_cmd_buff()
 *   7. Handle empty commands
 *   8. Check if built-in command, execute it
 *   9. If not built-in, fork/exec the external command
 *   10. Loop back to step 2
 */
int exec_local_cmd_loop()
{
    char cmd_line[SH_CMD_MAX];
    cmd_buff_t cmd;
    int rc;
    
    // Allocate cmd_buff
    rc = alloc_cmd_buff(&cmd);
    if (rc != OK) {
        return ERR_MEMORY;
    }
    
    while (1) {
        // Print the shell prompt
        printf("%s", SH_PROMPT);
        
        // Read a line of input
        if (fgets(cmd_line, SH_CMD_MAX, stdin) == NULL) {
            // EOF received (Ctrl+D)
            printf("\n");
            break;
        }
        
        // Remove trailing newline
        cmd_line[strcspn(cmd_line, "\n")] = '\0';
        
        // Check for exit command
        if (strcmp(cmd_line, EXIT_CMD) == 0) {
            printf("exiting...\n");
            break;
        }
        
        // Clear the command buffer and parse
        clear_cmd_buff(&cmd);
        rc = build_cmd_buff(cmd_line, &cmd);
        
        // Handle empty or parse error
        if (rc != OK || cmd.argc == 0) {
            continue;
        }
        
        // Check if built-in command
        Built_In_Cmds bi = exec_built_in_cmd(&cmd);
        
        if (bi == BI_CMD_EXIT) {
            printf("exiting...\n");
            break;
        }
        
        if (bi == BI_EXECUTED) {
            // Built-in was executed, continue to next command
            continue;
        }
        
        // Not a built-in command - fork/exec
        exec_cmd(&cmd);
    }
    
    // Free memory
    free_cmd_buff(&cmd);
    
    return OK;
}

