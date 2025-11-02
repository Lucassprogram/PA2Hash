#ifndef COMMANDS_H
#define COMMANDS_H

#include <stddef.h>
#include <stdint.h>

#include "hash_table.h"

typedef enum {
    COMMAND_INSERT,
    COMMAND_DELETE,
    COMMAND_SEARCH,
    COMMAND_PRINT
} CommandType;

typedef struct {
    CommandType type;
    char name[HASH_NAME_MAX + 1];
    uint32_t salary;
    uint32_t priority;
} Command;

typedef struct {
    Command *items;
    size_t size;
    size_t capacity;
} CommandList;

int load_commands(const char *path, CommandList *list, char *error_message, size_t error_size);
void free_command_list(CommandList *list);
const char *command_type_to_string(CommandType type);

#endif // COMMANDS_H
