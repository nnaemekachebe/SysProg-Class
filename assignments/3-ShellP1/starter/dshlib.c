#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "dshlib.h"

/*
 *  build_cmd_list
 *    cmd_line:     the command line from the user
 *    clist *:      pointer to clist structure to be populated
 *
 *  This function builds the command_list_t structure passed by the caller
 *  It does this by first splitting the cmd_line into commands by spltting
 *  the string based on any pipe characters '|'.  It then traverses each
 *  command.  For each command (a substring of cmd_line), it then parses
 *  that command by taking the first token as the executable name, and
 *  then the remaining tokens as the arguments.
 *
 *  NOTE your implementation should be able to handle properly removing
 *  leading and trailing spaces!
 *
 *  errors returned:
 *
 *    OK:                      No Error
 *    ERR_TOO_MANY_COMMANDS:   There is a limit of CMD_MAX (see dshlib.h)
 *                             commands.
 *    ERR_CMD_OR_ARGS_TOO_BIG: One of the commands provided by the user
 *                             was larger than allowed, either the
 *                             executable name, or the arg string.
 *
 *  Standard Library Functions You Might Want To Consider Using
 *      memset(), strcmp(), strcpy(), strtok(), strlen(), strchr()
 */

// Helper function to trim leading and trailing whitespace
static char *trim_whitespace(char *str)
{
    char *end;
    
    // Trim leading space
    while(isspace((unsigned char)*str)) str++;
    
    if(*str == 0) return str;
    
    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    
    // Write new null terminator
    end[1] = '\0';
    
    return str;
}

int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    // Check for NULL input
    if (cmd_line == NULL) {
        return WARN_NO_CMDS;
    }
    
    // Trim leading/trailing whitespace from input
    char *trimmed = trim_whitespace(cmd_line);
    
    // Check if empty or all whitespace
    if (strlen(trimmed) == 0) {
        return WARN_NO_CMDS;
    }
    
    // Initialize command list
    clist->num = 0;
    memset(clist->commands, 0, sizeof(clist->commands));
    
    // Make a copy of cmd_line since strtok modifies the string
    char *cmd_copy = malloc(strlen(cmd_line) + 1);
    if (cmd_copy == NULL) {
        return ERR_CMD_OR_ARGS_TOO_BIG;
    }
    strcpy(cmd_copy, cmd_line);
    
    // First pass: count pipe characters to check limit
    int pipe_count = 0;
    char *p = cmd_copy;
    while ((p = strchr(p, PIPE_CHAR)) != NULL) {
        pipe_count++;
        p++;
    }
    
    // Check if too many commands
    if (pipe_count >= CMD_MAX) {
        free(cmd_copy);
        return ERR_TOO_MANY_COMMANDS;
    }
    
    // Reset for second pass
    strcpy(cmd_copy, cmd_line);
    
    // Tokenize by pipe character
    char *saveptr1;
    char *cmd_segment = strtok_r(cmd_copy, PIPE_STRING, &saveptr1);
    
    while (cmd_segment != NULL && clist->num < CMD_MAX) {
        // Trim leading/trailing whitespace from each segment
        char *trimmed_segment = trim_whitespace(cmd_segment);
        
        // Parse the command segment
        // First token is the executable, rest are arguments
        char *saveptr2;
        char *token = strtok_r(trimmed_segment, " ", &saveptr2);
        
        if (token != NULL) {
            // First token is the command/executable
            strncpy(clist->commands[clist->num].exe, token, EXE_MAX - 1);
            clist->commands[clist->num].exe[EXE_MAX - 1] = '\0';
            
            // Get remaining tokens as arguments
            char args_buf[ARG_MAX] = {0};
            int first_arg = 1;
            
            while ((token = strtok_r(NULL, " ", &saveptr2)) != NULL) {
                if (!first_arg) {
                    strncat(args_buf, " ", ARG_MAX - strlen(args_buf) - 1);
                }
                strncat(args_buf, token, ARG_MAX - strlen(args_buf) - 1);
                first_arg = 0;
            }
            
            strncpy(clist->commands[clist->num].args, args_buf, ARG_MAX - 1);
            clist->commands[clist->num].args[ARG_MAX - 1] = '\0';
            
            clist->num++;
        }
        
        // Get next segment
        cmd_segment = strtok_r(NULL, PIPE_STRING, &saveptr1);
    }
    
    free(cmd_copy);
    
    // Check if we have any commands
    if (clist->num == 0) {
        return WARN_NO_CMDS;
    }
    
    return OK;
}

