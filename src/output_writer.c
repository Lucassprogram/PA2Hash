#include "output_writer.h"

#include <stdarg.h>
#include <stdlib.h>

int output_writer_init(OutputWriter *writer, const char *path) {
    if (!writer || !path) {
        return -1;
    }
    writer->fp = fopen(path, "w");
    if (!writer->fp) {
        return -1;
    }
    if (pthread_mutex_init(&writer->mutex, NULL) != 0) {
        fclose(writer->fp);
        writer->fp = NULL;
        return -1;
    }
    return 0;
}

void output_writer_close(OutputWriter *writer) {
    if (!writer) {
        return;
    }
    if (writer->fp) {
        fflush(writer->fp);
        fclose(writer->fp);
        writer->fp = NULL;
    }
    pthread_mutex_destroy(&writer->mutex);
}

int output_writer_append(OutputWriter *writer, const char *text) {
    if (!writer || !writer->fp || !text) {
        return -1;
    }
    pthread_mutex_lock(&writer->mutex);
    fputs(text, writer->fp);
    fflush(writer->fp);
    pthread_mutex_unlock(&writer->mutex);
    return 0;
}

int output_writer_appendf(OutputWriter *writer, const char *format, ...) {
    if (!writer || !writer->fp || !format) {
        return -1;
    }
    pthread_mutex_lock(&writer->mutex);
    va_list args;
    va_start(args, format);
    vfprintf(writer->fp, format, args);
    va_end(args);
    fflush(writer->fp);
    pthread_mutex_unlock(&writer->mutex);
    return 0;
}
