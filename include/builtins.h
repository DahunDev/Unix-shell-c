#ifndef BUILTINS_H
#define BUILTINS_H
// Execute built-in shell commands.
#include "shell.h"
int run_builtin(Pipeline *pipeline, bool *handled);
#endif
