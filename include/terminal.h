#ifndef TERMINAL_H
#define TERMINAL_H
// Manage terminal foreground process groups.
#include "shell.h"
#include <stdbool.h>
void terminal_init(void);
bool terminal_is_interactive(void);
pid_t terminal_shell_pgid(void);
int terminal_foreground(pid_t pgid);
#endif
