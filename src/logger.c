#include "logger.h"

#include <stdarg.h>
#include <stdlib.h>

#include "timestamp.h"

int logger_init(Logger *logger, const char *path) {
    if (!logger || !path) {
        return -1;
    }
    logger->fp = fopen(path, "w");
    if (!logger->fp) {
        return -1;
    }
    if (pthread_mutex_init(&logger->mutex, NULL) != 0) {
        fclose(logger->fp);
        logger->fp = NULL;
        return -1;
    }
    return 0;
}

void logger_close(Logger *logger) {
    if (!logger) {
        return;
    }
    if (logger->fp) {
        fflush(logger->fp);
        fclose(logger->fp);
        logger->fp = NULL;
    }
    pthread_mutex_destroy(&logger->mutex);
}

void logger_log_command(Logger *logger, uint32_t priority, const char *format, ...) {
    if (!logger || !logger->fp || !format) {
        return;
    }
    pthread_mutex_lock(&logger->mutex);
    long long timestamp = current_timestamp_microseconds();
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    fprintf(logger->fp, "%lld,THREAD %u,%s\n", timestamp, priority, buffer);
    fflush(logger->fp);
    pthread_mutex_unlock(&logger->mutex);
}

void logger_log_status(Logger *logger, uint32_t priority, const char *status) {
    if (!logger || !logger->fp || !status) {
        return;
    }
    pthread_mutex_lock(&logger->mutex);
    long long timestamp = current_timestamp_microseconds();
    fprintf(logger->fp, "%lld,THREAD %u%s\n", timestamp, priority, status);
    fflush(logger->fp);
    pthread_mutex_unlock(&logger->mutex);
}
