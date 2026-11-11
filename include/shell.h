#ifndef SHELL_H
#define SHELL_H
#define _POSIX_C_SOURCE 200809L
#include <stdbool.h>
#include <sys/types.h>
#define MAX_TOKENS 256
#define MAX_CMDS 32
#define MAX_JOBS 128

// Token types recognized by the command-line lexer.
typedef enum { WORD, PIPE, IN, OUT, APPEND, AMP } TokenType;
typedef struct { TokenType type; char *text; } Token;
// Store one executable command and its I/O redirections.
typedef struct {
    char *argv[MAX_TOKENS];
    int argc;
    char *input;
    char *output;
    bool append;
} Command;
// Represent a pipeline of commands and its execution mode.
typedef struct { Command commands[MAX_CMDS]; int count; bool background; } Pipeline;
#endif
