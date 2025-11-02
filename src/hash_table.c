#include "hash_table.h"

#include <stdlib.h>
#include <string.h>

void hash_table_init(HashTable *table) {
    if (!table) {
        return;
    }
    table->head = NULL;
    pthread_rwlock_init(&table->rwlock, NULL);
}

void hash_table_destroy(HashTable *table) {
    if (!table) {
        return;
    }
    pthread_rwlock_wrlock(&table->rwlock);
    hashRecord *current = table->head;
    while (current) {
        hashRecord *next = current->next;
        free(current);
        current = next;
    }
    table->head = NULL;
    pthread_rwlock_unlock(&table->rwlock);
    pthread_rwlock_destroy(&table->rwlock);
}

uint32_t jenkins_one_at_a_time_hash(const char *key) {
    uint32_t hash = 0;
    if (!key) {
        return hash;
    }
    while (*key) {
        hash += (unsigned char)(*key++);
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

hashRecord *hash_table_find(HashTable *table, const char *name) {
    if (!table || !name) {
        return NULL;
    }
    uint32_t target_hash = jenkins_one_at_a_time_hash(name);
    hashRecord *current = table->head;
    while (current) {
        if (current->hash == target_hash && strncmp(current->name, name, HASH_NAME_MAX) == 0) {
            return current;
        }
        if (current->hash > target_hash) {
            break;
        }
        current = current->next;
    }
    return NULL;
}

int hash_table_insert_locked(HashTable *table, const char *name, uint32_t salary,
                             uint32_t hash, uint32_t *prev_salary, int *was_update) {
    if (!table || !name) {
        return -1;
    }
    if (prev_salary) {
        *prev_salary = 0u;
    }
    if (was_update) {
        *was_update = 0;
    }
    hashRecord *prev = NULL;
    hashRecord *current = table->head;
    while (current && current->hash < hash) {
        prev = current;
        current = current->next;
    }
    while (current && current->hash == hash) {
        int name_cmp = strncmp(current->name, name, HASH_NAME_MAX);
        if (name_cmp == 0) {
            if (prev_salary) {
                *prev_salary = current->salary;
            }
            current->salary = salary;
            if (was_update) {
                *was_update = 1;
            }
            return 0;
        }
        prev = current;
        current = current->next;
    }

    hashRecord *node = (hashRecord *)calloc(1, sizeof(hashRecord));
    if (!node) {
        return -1;
    }
    node->hash = hash;
    strncpy(node->name, name, HASH_NAME_MAX);
    node->name[HASH_NAME_MAX] = '\0';
    node->salary = salary;

    if (!prev) {
        node->next = table->head;
        table->head = node;
    } else {
        node->next = prev->next;
        prev->next = node;
    }
    return 0;
}

int hash_table_delete_locked(HashTable *table, const char *name, uint32_t hash,
                             uint32_t *removed_salary) {
    if (!table || !name) {
        return -1;
    }
    hashRecord *prev = NULL;
    hashRecord *current = table->head;
    while (current && current->hash < hash) {
        prev = current;
        current = current->next;
    }
    while (current && current->hash == hash) {
        if (strncmp(current->name, name, HASH_NAME_MAX) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                table->head = current->next;
            }
            if (removed_salary) {
                *removed_salary = current->salary;
            }
            free(current);
            return 1;
        }
        prev = current;
        current = current->next;
    }
    return 0;
}

hashRecord *hash_table_clone_records(HashTable *table, size_t *out_count) {
    if (!table || !out_count) {
        return NULL;
    }
    size_t count = 0;
    for (hashRecord *current = table->head; current; current = current->next) {
        ++count;
    }
    *out_count = count;
    if (count == 0) {
        return NULL;
    }
    hashRecord *records = (hashRecord *)calloc(count, sizeof(hashRecord));
    if (!records) {
        *out_count = 0;
        return NULL;
    }
    size_t index = 0;
    for (hashRecord *current = table->head; current && index < count; current = current->next) {
        records[index] = *current;
        records[index].next = NULL;
        ++index;
    }
    return records;
}
