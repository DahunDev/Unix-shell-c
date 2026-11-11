#include "lexer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Release memory allocated for token text.
void free_tokens(Token *tokens, int n) {
    for (int i = 0; i < n; i++) free(tokens[i].text);
}
// Append one token while enforcing the token limit.
static int push_token(Token *out, int *count, TokenType type, char *text) {
    if (*count >= MAX_TOKENS) { free(text); fprintf(stderr, "shell: too many tokens\n"); return -1; }
    out[*count] = (Token){type, text}; ++*count; return 0;
}

// Split input into words and operators while honoring quotes.
int tokenize(const char *line, Token *out) {
    int n = 0;
    for (size_t i = 0; line[i]; ) {
        if (isspace((unsigned char)line[i])) { i++; continue; }
        TokenType op;
        int length = 1;
        switch (line[i]) {
            case '|': op = PIPE; break;
            case '<': op = IN; break;
            case '>': op = OUT; if (line[i+1] == '>') { op = APPEND; length = 2; } break;
            case '&': op = AMP; break;
            default: goto word;
        }
        if (push_token(out, &n, op, NULL) < 0) goto fail;
        i += (size_t)length;
        continue;
word:;
        char *buf = malloc(strlen(line + i) + 1);
        if (!buf) goto fail;
        size_t k = 0;
        char quote = '\0';
        while (line[i]) {
            char c = line[i];
            if (quote) {
                if (c == quote) { quote = '\0'; i++; continue; }
                // Preserve backslashes before ordinary double-quoted characters.
                if (c == '\\' && quote == '"' && line[i+1]) {
                    char next = line[i+1];
                    if (next == '$' || next == '`' || next == '"' || next == '\\') {
                        buf[k++] = next;
                        i += 2;
                        continue;
                    }
                }
                buf[k++] = c; i++; continue;
            }
            if (c == '\'' || c == '"') { quote = c; i++; continue; }
            if (c == '\\' && line[i+1]) { buf[k++] = line[i+1]; i += 2; continue; }
            if (isspace((unsigned char)c) || c == '|' || c == '<' || c == '>' || c == '&') break;
            buf[k++] = c; i++;
        }
        if (quote) { fprintf(stderr, "shell: unmatched quote\n"); free(buf); goto fail; }
        buf[k] = '\0';
        if (push_token(out, &n, WORD, buf) < 0) goto fail;
    }
    return n;
fail:
    free_tokens(out, n);
    return -1;
}

