#ifndef LOGGER_H
#define LOGGER_H

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    FILE *fp;
    pthread_mutex_t mutex;
} Logger;

int logger_init(Logger *logger, const char *path);
void logger_close(Logger *logger);
void logger_log_command(Logger *logger, uint32_t priority, const char *format, ...);
void logger_log_status(Logger *logger, uint32_t priority, const char *status);

#endif // LOGGER_H
