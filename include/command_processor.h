#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <pthread.h>

#include "commands.h"
#include "hash_table.h"
#include "logger.h"
#include "output_writer.h"

typedef struct {
    HashTable *table;
    Logger *logger;
    OutputWriter *output;
    Command command;
} CommandContext;

void *command_worker(void *arg);

#endif // COMMAND_PROCESSOR_H
