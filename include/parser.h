#ifndef PARSER_H
#define PARSER_H
// Convert tokens into a pipeline.
#include "shell.h"
int parse(Token *tokens, int n, Pipeline *pipeline);
#endif
