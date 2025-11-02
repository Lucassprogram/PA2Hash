#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command_processor.h"
#include "commands.h"
#include "hash_table.h"
#include "logger.h"
#include "output_writer.h"

#define COMMANDS_FILE "commands.txt"
#define OUTPUT_FILE "output.txt"
#define LOG_FILE "hash.log"

int main(void) {
    HashTable table;
    hash_table_init(&table);

    Logger logger;
    if (logger_init(&logger, LOG_FILE) != 0) {
        fprintf(stderr, "Failed to initialize logger at %s\n", LOG_FILE);
        return EXIT_FAILURE;
    }

    OutputWriter output;
    if (output_writer_init(&output, OUTPUT_FILE) != 0) {
        fprintf(stderr, "Failed to initialize output writer at %s\n", OUTPUT_FILE);
        logger_close(&logger);
        return EXIT_FAILURE;
    }

    CommandList commands;
    char error_buffer[256];
    if (load_commands(COMMANDS_FILE, &commands, error_buffer, sizeof(error_buffer)) != 0) {
        fprintf(stderr, "Error loading commands: %s\n", error_buffer);
        output_writer_close(&output);
        logger_close(&logger);
        hash_table_destroy(&table);
        return EXIT_FAILURE;
    }

    if (commands.size == 0) {
        printf("No commands to execute.\n");
        free_command_list(&commands);
        output_writer_close(&output);
        logger_close(&logger);
        hash_table_destroy(&table);
        return EXIT_SUCCESS;
    }

    pthread_t *threads = (pthread_t *)calloc(commands.size, sizeof(pthread_t));
    int *thread_created = (int *)calloc(commands.size, sizeof(int));
    CommandContext *contexts = (CommandContext *)calloc(commands.size, sizeof(CommandContext));
    if (!threads || !thread_created || !contexts) {
        fprintf(stderr, "Failed to allocate thread resources.\n");
        free(threads);
        free(thread_created);
        free(contexts);
        free_command_list(&commands);
        output_writer_close(&output);
        logger_close(&logger);
        hash_table_destroy(&table);
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < commands.size; ++i) {
        contexts[i].table = &table;
        contexts[i].logger = &logger;
        contexts[i].output = &output;
        contexts[i].command = commands.items[i];
        int rc = pthread_create(&threads[i], NULL, command_worker, &contexts[i]);
        if (rc != 0) {
            fprintf(stderr, "Failed to create thread for command %zu, executing synchronously.\n", i);
            command_worker(&contexts[i]);
            thread_created[i] = 0;
        } else {
            thread_created[i] = 1;
        }
    }

    for (size_t i = 0; i < commands.size; ++i) {
        if (thread_created[i]) {
            pthread_join(threads[i], NULL);
        }
    }

    free(contexts);
    free(thread_created);
    free(threads);
    free_command_list(&commands);
    output_writer_close(&output);
    logger_close(&logger);
    hash_table_destroy(&table);
    return EXIT_SUCCESS;
}
