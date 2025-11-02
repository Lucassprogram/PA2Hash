#include "commands.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void trim_whitespace(char *text) {
    if (!text) {
        return;
    }
    char *start = text;
    while (*start && isspace((unsigned char)*start)) {
        ++start;
    }
    char *end = start + strlen(start);
    while (end > start && isspace((unsigned char)*(end - 1))) {
        --end;
    }
    *end = '\0';
    if (start != text) {
        memmove(text, start, end - start + 1);
    }
}

static int ensure_capacity(CommandList *list, size_t min_capacity) {
    if (list->capacity >= min_capacity) {
        return 0;
    }
    size_t new_capacity = list->capacity ? list->capacity * 2 : 8;
    if (new_capacity < min_capacity) {
        new_capacity = min_capacity;
    }
    Command *resized = (Command *)realloc(list->items, new_capacity * sizeof(Command));
    if (!resized) {
        return -1;
    }
    list->items = resized;
    list->capacity = new_capacity;
    return 0;
}

static int parse_unsigned(const char *token, uint32_t *value) {
    if (!token || !value) {
        return -1;
    }
    errno = 0;
    char *endptr = NULL;
    unsigned long parsed = strtoul(token, &endptr, 10);
    if (errno != 0 || endptr == token || *endptr != '\0') {
        return -1;
    }
    if (parsed > UINT32_MAX) {
        return -1;
    }
    *value = (uint32_t)parsed;
    return 0;
}

static int parse_line(const char *raw_line, Command *command, char *error_message, size_t error_size) {
    if (!raw_line || !command) {
        return -1;
    }
    char buffer[256];
    strncpy(buffer, raw_line, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    trim_whitespace(buffer);
    if (buffer[0] == '\0' || buffer[0] == '#') {
        return 1; // skip empty/comment
    }

    char *tokens[4] = {0};
    size_t token_count = 0;
    char *context = NULL;
    char *token = strtok_r(buffer, ",", &context);
    while (token && token_count < 4) {
        trim_whitespace(token);
        tokens[token_count++] = token;
        token = strtok_r(NULL, ",", &context);
    }
    if (!tokens[0]) {
        snprintf(error_message, error_size, "Missing command token");
        return -1;
    }

    char command_token[32];
    strncpy(command_token, tokens[0], sizeof(command_token) - 1);
    command_token[sizeof(command_token) - 1] = '\0';
    for (size_t i = 0; command_token[i]; ++i) {
        command_token[i] = (char)toupper((unsigned char)command_token[i]);
    }

    if (strcmp(command_token, "INSERT") == 0) {
        if (token_count < 4) {
            snprintf(error_message, error_size, "INSERT expects 4 tokens");
            return -1;
        }
        if (strlen(tokens[1]) > HASH_NAME_MAX) {
            snprintf(error_message, error_size, "Name exceeds %d characters", HASH_NAME_MAX);
            return -1;
        }
        uint32_t salary;
        if (parse_unsigned(tokens[2], &salary) != 0) {
            snprintf(error_message, error_size, "Invalid salary value");
            return -1;
        }
        uint32_t priority;
        if (parse_unsigned(tokens[3], &priority) != 0) {
            snprintf(error_message, error_size, "Invalid priority value");
            return -1;
        }
        command->type = COMMAND_INSERT;
        strncpy(command->name, tokens[1], HASH_NAME_MAX);
        command->name[HASH_NAME_MAX] = '\0';
        command->salary = salary;
        command->priority = priority;
        return 0;
    }

    if (strcmp(command_token, "DELETE") == 0) {
        if (token_count < 3) {
            snprintf(error_message, error_size, "DELETE expects 3 tokens");
            return -1;
        }
        if (strlen(tokens[1]) > HASH_NAME_MAX) {
            snprintf(error_message, error_size, "Name exceeds %d characters", HASH_NAME_MAX);
            return -1;
        }
        uint32_t priority;
        if (parse_unsigned(tokens[2], &priority) != 0) {
            snprintf(error_message, error_size, "Invalid priority value");
            return -1;
        }
        command->type = COMMAND_DELETE;
        strncpy(command->name, tokens[1], HASH_NAME_MAX);
        command->name[HASH_NAME_MAX] = '\0';
        command->salary = 0;
        command->priority = priority;
        return 0;
    }

    if (strcmp(command_token, "SEARCH") == 0) {
        if (token_count < 3) {
            snprintf(error_message, error_size, "SEARCH expects 3 tokens");
            return -1;
        }
        if (strlen(tokens[1]) > HASH_NAME_MAX) {
            snprintf(error_message, error_size, "Name exceeds %d characters", HASH_NAME_MAX);
            return -1;
        }
        uint32_t priority;
        if (parse_unsigned(tokens[2], &priority) != 0) {
            snprintf(error_message, error_size, "Invalid priority value");
            return -1;
        }
        command->type = COMMAND_SEARCH;
        strncpy(command->name, tokens[1], HASH_NAME_MAX);
        command->name[HASH_NAME_MAX] = '\0';
        command->salary = 0;
        command->priority = priority;
        return 0;
    }

    if (strcmp(command_token, "PRINT") == 0) {
        if (token_count < 2) {
            snprintf(error_message, error_size, "PRINT expects 2 tokens");
            return -1;
        }
        uint32_t priority;
        if (parse_unsigned(tokens[1], &priority) != 0) {
            snprintf(error_message, error_size, "Invalid priority value");
            return -1;
        }
        command->type = COMMAND_PRINT;
        command->name[0] = '\0';
        command->salary = 0;
        command->priority = priority;
        return 0;
    }

    snprintf(error_message, error_size, "Unknown command '%s'", tokens[0]);
    return -1;
}

int load_commands(const char *path, CommandList *list, char *error_message, size_t error_size) {
    if (!path || !list) {
        if (error_message && error_size > 0) {
            snprintf(error_message, error_size, "Invalid arguments");
        }
        return -1;
    }
    FILE *fp = fopen(path, "r");
    if (!fp) {
        if (error_message && error_size > 0) {
            snprintf(error_message, error_size, "Unable to open %s", path);
        }
        return -1;
    }
    list->items = NULL;
    list->size = 0;
    list->capacity = 0;
    char line[256];
    size_t line_number = 0;
    while (fgets(line, sizeof(line), fp)) {
        ++line_number;
        Command command;
        char parse_error[128] = {0};
        int result = parse_line(line, &command, parse_error, sizeof(parse_error));
        if (result < 0) {
            if (error_message && error_size > 0) {
                snprintf(error_message, error_size, "Line %zu: %s", line_number, parse_error);
            }
            fclose(fp);
            free(list->items);
            list->items = NULL;
            list->size = 0;
            list->capacity = 0;
            return -1;
        }
        if (result > 0) {
            continue; // skip comments/empty
        }
        if (ensure_capacity(list, list->size + 1) != 0) {
            if (error_message && error_size > 0) {
                snprintf(error_message, error_size, "Out of memory");
            }
            fclose(fp);
            free(list->items);
            list->items = NULL;
            list->size = 0;
            list->capacity = 0;
            return -1;
        }
        list->items[list->size++] = command;
    }
    fclose(fp);
    return 0;
}

void free_command_list(CommandList *list) {
    if (!list) {
        return;
    }
    free(list->items);
    list->items = NULL;
    list->size = 0;
    list->capacity = 0;
}

const char *command_type_to_string(CommandType type) {
    switch (type) {
        case COMMAND_INSERT:
            return "INSERT";
        case COMMAND_DELETE:
            return "DELETE";
        case COMMAND_SEARCH:
            return "SEARCH";
        case COMMAND_PRINT:
            return "PRINT";
        default:
            return "UNKNOWN";
    }
}

