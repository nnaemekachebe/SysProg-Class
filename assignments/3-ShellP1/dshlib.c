#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "dshlib.h"

//===================================================================
// HELPER FUNCTIONS - Memory Management (PROVIDED)
//===================================================================

/**
 * alloc_cmd_buff - Allocate memory for cmd_buff internal buffer
 * 
 * This function is provided for you. It allocates the _cmd_buffer
 * that will store the command string.
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
 * 
 * This function is provided for you. Call it when done with a cmd_buff.
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
 * 
 * This function is provided for you.
 */
int clear_cmd_buff(cmd_buff_t *cmd_buff)
{
    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    return OK;
}

/**
 * free_cmd_list - Free all cmd_buffs in a command list
 * 
 * This function is provided for you. It frees all allocated memory
 * in a command_list_t structure.
 */
int free_cmd_list(command_list_t *cmd_lst)
{
    for (int i = 0; i < cmd_lst->num; i++) {
        free_cmd_buff(&cmd_lst->commands[i]);
    }
    cmd_lst->num = 0;
    return OK;
}

//===================================================================
// PARSING FUNCTIONS - YOU IMPLEMENT THESE
//===================================================================

/**
 * trim_whitespace - Helper function to trim leading and trailing whitespace
 */
static char *trim_whitespace(char *str)
{
    char *end;
    
    // Trim leading space
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) return str;
    
    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    // Write new null terminator
    end[1] = '\0';
    
    return str;
}

/**
 * build_cmd_buff - Parse a single command string into cmd_buff_t
 */
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff)
{
    // Allocate internal buffer
    if (alloc_cmd_buff(cmd_buff) != OK) {
        return ERR_MEMORY;
    }
    
    // Copy cmd_line to internal buffer (strtok modifies the string)
    strncpy(cmd_buff->_cmd_buffer, cmd_line, SH_CMD_MAX - 1);
    cmd_buff->_cmd_buffer[SH_CMD_MAX - 1] = '\0';
    
    // Trim whitespace from the command
    char *trimmed = trim_whitespace(cmd_buff->_cmd_buffer);
    
    // Parse tokens with quote handling
    int argc = 0;
    char *p = trimmed;
    
    while (*p && argc < CMD_ARGV_MAX - 1) {
        // Skip leading spaces
        while (*p && *p == ' ') p++;
        if (!*p) break;
        
        // Check for quotes
        char quote_char = 0;
        if (*p == '"' || *p == '\'') {
            quote_char = *p;
            p++;  // Skip opening quote
        }
        
        // Find end of token
        char *token_start = p;
        if (quote_char) {
            // Find closing quote
            while (*p && *p != quote_char) p++;
            // Remove closing quote if found
            if (*p == quote_char) *p++ = '\0';
        } else {
            // Find space or end
            while (*p && *p != ' ') p++;
            if (*p) *p++ = '\0';
        }
        
        // Store token
        cmd_buff->argv[argc] = token_start;
        argc++;
    }
    
    // NULL terminate the argv array (required for execvp)
    cmd_buff->argv[argc] = NULL;
    cmd_buff->argc = argc;
    
    return OK;
}

/**
 * build_cmd_list - Parse command line with pipes into command_list_t
 */
int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    // Check for NULL input
    if (cmd_line == NULL) {
        return WARN_NO_CMDS;
    }
    
    // Make a copy since strtok modifies the string
    char *cmd_copy = malloc(strlen(cmd_line) + 1);
    if (cmd_copy == NULL) {
        return ERR_MEMORY;
    }
    strcpy(cmd_copy, cmd_line);
    
    // Trim leading/trailing whitespace from input
    char *trimmed = trim_whitespace(cmd_copy);
    
    // Check if empty or all whitespace
    if (strlen(trimmed) == 0) {
        free(cmd_copy);
        return WARN_NO_CMDS;
    }
    
    // Initialize command list
    clist->num = 0;
    
    // First pass: count pipe characters to check limit
    int pipe_count = 0;
    char *p = trimmed;
    while ((p = strchr(p, PIPE_CHAR)) != NULL) {
        pipe_count++;
        p++;
    }
    
    // Check if too many commands (CMD_MAX commands means CMD_MAX-1 pipes)
    if (pipe_count >= CMD_MAX) {
        free(cmd_copy);
        return ERR_TOO_MANY_COMMANDS;
    }
    
    // Reset for second pass - copy original again
    strcpy(cmd_copy, cmd_line);
    trimmed = trim_whitespace(cmd_copy);
    
    // Tokenize by pipe character
    char *saveptr;
    char *cmd_segment = strtok_r(trimmed, PIPE_STRING, &saveptr);
    
    while (cmd_segment != NULL && clist->num < CMD_MAX) {
        // Trim leading/trailing whitespace from each segment
        char *trimmed_segment = trim_whitespace(cmd_segment);
        
        // Allocate cmd_buff for this command
        if (alloc_cmd_buff(&clist->commands[clist->num]) != OK) {
            // Free already allocated commands
            for (int i = 0; i < clist->num; i++) {
                free_cmd_buff(&clist->commands[i]);
            }
            free(cmd_copy);
            return ERR_MEMORY;
        }
        
        // Parse the segment
        if (build_cmd_buff(trimmed_segment, &clist->commands[clist->num]) != OK) {
            for (int i = 0; i <= clist->num; i++) {
                free_cmd_buff(&clist->commands[i]);
            }
            free(cmd_copy);
            return ERR_MEMORY;
        }
        
        clist->num++;
        
        // Get next segment
        cmd_segment = strtok_r(NULL, PIPE_STRING, &saveptr);
    }
    
    free(cmd_copy);
    
    // Check if we have any commands
    if (clist->num == 0) {
        return WARN_NO_CMDS;
    }
    
    return OK;
}

//===================================================================
// BUILT-IN COMMAND FUNCTIONS (PROVIDED FOR PART 1)
//===================================================================

/**
 * match_command - Check if input is a built-in command
 * 
 * This function is provided for you.
 */
Built_In_Cmds match_command(const char *input)
{
    if (strcmp(input, EXIT_CMD) == 0) {
        return BI_CMD_EXIT;
    }
    if (strcmp(input, "dragon") == 0) {
        return BI_CMD_DRAGON;
    }
    if (strcmp(input, "cd") == 0) {
        return BI_CMD_CD;
    }
    return BI_NOT_BI;
}

/**
 * exec_built_in_cmd - Execute built-in commands
 * 
 * This function is provided for you, but incomplete.
 * You can add dragon command here for extra credit.
 */
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd)
{
    Built_In_Cmds bi_cmd = match_command(cmd->argv[0]);
    
    switch (bi_cmd) {
        case BI_CMD_EXIT:
            // Exit is handled in main loop
            return BI_CMD_EXIT;
            
        case BI_CMD_DRAGON:
            // TODO: Extra credit - implement dragon here
            printf("Dragon not implemented yet!\n");
            return BI_EXECUTED;
            
        case BI_CMD_CD:
            // CD will be implemented in Part 2
            printf("cd not implemented yet!\n");
            return BI_EXECUTED;
            
        default:
            return BI_NOT_BI;
    }
}

//===================================================================
// MAIN SHELL LOOP - YOU IMPLEMENT THIS
//===================================================================

/**
 * exec_local_cmd_loop - Main shell loop
 * 
 * YOU NEED TO IMPLEMENT THIS FUNCTION! This is your shell's main loop.
 * 
 * This function should:
 *   1. Loop forever (while(1))
 *   2. Print the shell prompt (SH_PROMPT)
 *   3. Read a line of input using fgets()
 *   4. Remove trailing newline
 *   5. Check for exit command - if found, print "exiting..." and break
 *   6. Parse the command line using build_cmd_list()
 *   7. Handle return codes:
 *      - WARN_NO_CMDS: print CMD_WARN_NO_CMD
 *      - ERR_TOO_MANY_COMMANDS: print CMD_ERR_PIPE_LIMIT with CMD_MAX
 *      - OK: print the parsed commands (see below)
 *   8. Free the command list using free_cmd_list()
 *   9. Loop back to step 2
 * 
 * Output format when commands are parsed successfully:
 * 
 *   First line:
 *     printf(CMD_OK_HEADER, clist.num);
 * 
 *   For each command:
 *     printf("<%d> %s", i+1, clist.commands[i].argv[0]);
 *     if (clist.commands[i].argc > 1) {
 *         printf(" [");
 *         for (int j = 1; j < clist.commands[i].argc; j++) {
 *             printf("%s", clist.commands[i].argv[j]);
 *             if (j < clist.commands[i].argc - 1) {
 *                 printf(" ");
 *             }
 *         }
 *         printf("]");
 *     }
 *     printf("\n");
 * 
 * Examples of expected output:
 * 
 *   dsh> cmd
 *   PARSED COMMAND LINE - TOTAL COMMANDS 1
 *   <1> cmd
 * 
 *   dsh> cmd arg1 arg2
 *   PARSED COMMAND LINE - TOTAL COMMANDS 1
 *   <1> cmd [arg1 arg2]
 * 
 *   dsh> cmd1 | cmd2 | cmd3
 *   PARSED COMMAND LINE - TOTAL COMMANDS 3
 *   <1> cmd1
 *   <2> cmd2
 *   <3> cmd3
 * 
 *   dsh> 
 *   warning: no commands provided
 * 
 *   dsh> c1|c2|c3|c4|c5|c6|c7|c8|c9
 *   error: piping limited to 8 commands
 * 
 *   dsh> exit
 *   exiting...
 * 
 * Starter code to help you get going:
 * 
 *   char cmd_line[SH_CMD_MAX];
 *   command_list_t clist;
 *   int rc;
 *   
 *   while (1) {
 *       printf("%s", SH_PROMPT);
 *       
 *       if (fgets(cmd_line, SH_CMD_MAX, stdin) == NULL) {
 *           printf("\n");
 *           break;
 *       }
 *       
 *       // Remove trailing newline
 *       cmd_line[strcspn(cmd_line, "\n")] = '\0';
 *       
 *       // Check for exit command
 *       // TODO: implement exit check
 *       
 *       // Parse the command line
 *       rc = build_cmd_list(cmd_line, &clist);
 *       
 *       // Handle return codes and print output
 *       // TODO: implement return code handling
 *       
 *       // Free memory
 *       if (rc == OK) {
 *           free_cmd_list(&clist);
 *       }
 *   }
 *   
 *   return OK;
 * 
 * @return: OK on success
 */
int exec_local_cmd_loop()
{
    char cmd_line[SH_CMD_MAX];
    command_list_t clist;
    int rc;
    
    while (1) {
        // Print prompt
        printf("%s", SH_PROMPT);
        
        // Read input
        if (fgets(cmd_line, SH_CMD_MAX, stdin) == NULL) {
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
        
        // Parse the command line
        rc = build_cmd_list(cmd_line, &clist);
        
        // Handle return codes and print output
        if (rc == OK) {
            // Print header
            printf(CMD_OK_HEADER, clist.num);
            
            // Print each command
            for (int i = 0; i < clist.num; i++) {
                printf("<%d> %s", i + 1, clist.commands[i].argv[0]);
                
                // Print arguments if present
                if (clist.commands[i].argc > 1) {
                    printf(" [");
                    for (int j = 1; j < clist.commands[i].argc; j++) {
                        printf("%s", clist.commands[i].argv[j]);
                        if (j < clist.commands[i].argc - 1) {
                            printf(" ");
                        }
                    }
                    printf("]");
                }
                printf("\n");
            }
            
            // Free memory
            free_cmd_list(&clist);
        }
        else if (rc == WARN_NO_CMDS) {
            printf(CMD_WARN_NO_CMD);
        }
        else if (rc == ERR_TOO_MANY_COMMANDS) {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
        }
    }
    
    return OK;
}

//===================================================================
// EXECUTION FUNCTIONS - For future assignments
//===================================================================

/**
 * exec_cmd - Execute a single command (Part 2)
 * 
 * This will be implemented in Part 2 using fork/exec
 */
int exec_cmd(cmd_buff_t *cmd)
{
    printf("exec_cmd not implemented yet (Part 2)\n");
    return OK;
}

/**
 * execute_pipeline - Execute piped commands (Part 3)
 * 
 * This will be implemented in Part 3 using pipes
 */
int execute_pipeline(command_list_t *clist)
{
    printf("execute_pipeline not implemented yet (Part 3)\n");
    return OK;
}