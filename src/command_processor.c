#include "command_processor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "timestamp.h"

static void log_waiting(CommandContext *ctx) {
    if (ctx->logger) {
        logger_log_status(ctx->logger, ctx->command.priority, "WAITING FOR MY TURN");
    }
}

static void log_awakened(CommandContext *ctx) {
    if (ctx->logger) {
        logger_log_status(ctx->logger, ctx->command.priority, "AWAKENED FOR WORK");
    }
}

static void log_read_acquired(CommandContext *ctx) {
    if (ctx->logger) {
        logger_log_status(ctx->logger, ctx->command.priority, "READ LOCK ACQUIRED");
    }
}

static void log_read_released(CommandContext *ctx) {
    if (ctx->logger) {
        logger_log_status(ctx->logger, ctx->command.priority, "READ LOCK RELEASED");
    }
}

static void log_write_acquired(CommandContext *ctx) {
    if (ctx->logger) {
        logger_log_status(ctx->logger, ctx->command.priority, "WRITE LOCK ACQUIRED");
    }
}

static void log_write_released(CommandContext *ctx) {
    if (ctx->logger) {
        logger_log_status(ctx->logger, ctx->command.priority, "WRITE LOCK RELEASED");
    }
}

static void acquire_read_lock(CommandContext *ctx) {
    log_waiting(ctx);
    pthread_rwlock_rdlock(&ctx->table->rwlock);
    log_awakened(ctx);
    log_read_acquired(ctx);
}

static void release_read_lock(CommandContext *ctx) {
    pthread_rwlock_unlock(&ctx->table->rwlock);
    log_read_released(ctx);
}

static void acquire_write_lock(CommandContext *ctx) {
    log_waiting(ctx);
    pthread_rwlock_wrlock(&ctx->table->rwlock);
    log_awakened(ctx);
    log_write_acquired(ctx);
}

static void release_write_lock(CommandContext *ctx) {
    pthread_rwlock_unlock(&ctx->table->rwlock);
    log_write_released(ctx);
}

static void process_insert(CommandContext *ctx) {
    uint32_t hash = jenkins_one_at_a_time_hash(ctx->command.name);
    if (ctx->logger) {
        logger_log_command(ctx->logger, ctx->command.priority, "INSERT,%u,%s,%u", hash, ctx->command.name, ctx->command.salary);
    }
    acquire_write_lock(ctx);
    uint32_t previous_salary = 0;
    int was_update = 0;
    int status = hash_table_insert_locked(ctx->table, ctx->command.name, ctx->command.salary, hash, &previous_salary, &was_update);
    release_write_lock(ctx);
    if (status != 0) {
        fprintf(stderr, "Failed to insert %s\n", ctx->command.name);
        return;
    }
    if (was_update) {
        printf("Updated record %u from %u to %u\n", hash, previous_salary, ctx->command.salary);
    } else {
        printf("Inserted %s with hash %u salary %u\n", ctx->command.name, hash, ctx->command.salary);
    }
}

static void process_delete(CommandContext *ctx) {
    uint32_t hash = jenkins_one_at_a_time_hash(ctx->command.name);
    if (ctx->logger) {
        logger_log_command(ctx->logger, ctx->command.priority, "DELETE,%u,%s", hash, ctx->command.name);
    }
    acquire_write_lock(ctx);
    uint32_t removed_salary = 0;
    int status = hash_table_delete_locked(ctx->table, ctx->command.name, hash, &removed_salary);
    release_write_lock(ctx);
    if (status == 1) {
        printf("Deleted record for %s (hash %u)\n", ctx->command.name, hash);
    } else {
        printf("No record found for %s\n", ctx->command.name);
    }
}

static void process_search(CommandContext *ctx) {
    uint32_t hash = jenkins_one_at_a_time_hash(ctx->command.name);
    if (ctx->logger) {
        logger_log_command(ctx->logger, ctx->command.priority, "SEARCH,%u,%s", hash, ctx->command.name);
    }
    acquire_read_lock(ctx);
    hashRecord *record = hash_table_find(ctx->table, ctx->command.name);
    hashRecord snapshot;
    int found = 0;
    if (record) {
        snapshot = *record;
        snapshot.next = NULL;
        found = 1;
    }
    release_read_lock(ctx);
    if (found) {
        printf("Found: %u,%s,%u\n", snapshot.hash, snapshot.name, snapshot.salary);
        if (ctx->output) {
            output_writer_appendf(ctx->output, "Found: %u,%s,%u\n", snapshot.hash, snapshot.name, snapshot.salary);
        }
    } else {
        printf("No Record Found\n");
        if (ctx->output) {
            output_writer_appendf(ctx->output, "No Record Found for %s\n", ctx->command.name);
        }
    }
}

static void process_print(CommandContext *ctx) {
    if (ctx->logger) {
        logger_log_command(ctx->logger, ctx->command.priority, "PRINT");
    }
    acquire_read_lock(ctx);
    size_t count = 0;
    hashRecord *records = hash_table_clone_records(ctx->table, &count);
    release_read_lock(ctx);

    printf("Current Database:\n");
    if (ctx->output) {
        output_writer_append(ctx->output, "Current Database:\n");
    }

    if (!records || count == 0) {
        printf("(empty)\n");
        if (ctx->output) {
            output_writer_append(ctx->output, "(empty)\n");
        }
    } else {
        for (size_t i = 0; i < count; ++i) {
            printf("%u,%s,%u\n", records[i].hash, records[i].name, records[i].salary);
            if (ctx->output) {
                output_writer_appendf(ctx->output, "%u,%s,%u\n", records[i].hash, records[i].name, records[i].salary);
            }
        }
        free(records);
    }
}

void *command_worker(void *arg) {
    CommandContext *ctx = (CommandContext *)arg;
    if (!ctx || !ctx->table) {
        return NULL;
    }
    switch (ctx->command.type) {
        case COMMAND_INSERT:
            process_insert(ctx);
            break;
        case COMMAND_DELETE:
            process_delete(ctx);
            break;
        case COMMAND_SEARCH:
            process_search(ctx);
            break;
        case COMMAND_PRINT:
            process_print(ctx);
            break;
        default:
            fprintf(stderr, "Unknown command type encountered\n");
            break;
    }
    return NULL;
}



