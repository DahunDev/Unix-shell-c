#ifndef LEXER_H
#define LEXER_H
// Tokenize command text and release tokens.
#include "shell.h"
void free_tokens(Token *tokens, int n);
int tokenize(const char *line, Token *out);
#endif
