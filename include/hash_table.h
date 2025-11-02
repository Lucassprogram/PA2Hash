#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdint.h>
#include <pthread.h>

#define HASH_NAME_MAX 50

typedef struct hash_struct {
    uint32_t hash;
    char name[HASH_NAME_MAX + 1];
    uint32_t salary;
    struct hash_struct *next;
} hashRecord;

typedef struct {
    hashRecord *head;
    pthread_rwlock_t rwlock;
} HashTable;

void hash_table_init(HashTable *table);
void hash_table_destroy(HashTable *table);

uint32_t jenkins_one_at_a_time_hash(const char *key);

hashRecord *hash_table_find(HashTable *table, const char *name);
int hash_table_insert_locked(HashTable *table, const char *name, uint32_t salary,
                             uint32_t hash, uint32_t *prev_salary, int *was_update);
int hash_table_delete_locked(HashTable *table, const char *name, uint32_t hash,
                             uint32_t *removed_salary);

hashRecord *hash_table_clone_records(HashTable *table, size_t *out_count);

#endif // HASH_TABLE_H
