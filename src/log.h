#ifndef LOG_H
#define LOG_H

#include <stdio.h>

void log_init(const char *filename);
void log_message(int process_id, const char *action, const char *details);
void log_close(void);

#endif