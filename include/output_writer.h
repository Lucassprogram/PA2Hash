#ifndef OUTPUT_WRITER_H
#define OUTPUT_WRITER_H

#include <pthread.h>
#include <stdio.h>

typedef struct {
    FILE *fp;
    pthread_mutex_t mutex;
} OutputWriter;

int output_writer_init(OutputWriter *writer, const char *path);
void output_writer_close(OutputWriter *writer);
int output_writer_append(OutputWriter *writer, const char *text);
int output_writer_appendf(OutputWriter *writer, const char *format, ...);

#endif // OUTPUT_WRITER_H
