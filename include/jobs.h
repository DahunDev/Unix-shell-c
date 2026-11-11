#ifndef JOBS_H
#define JOBS_H
// Track and reap background processes.
#include "shell.h"
void reap_jobs(void);
void add_job(pid_t pid, pid_t pgid);
void print_jobs(void);
#endif
