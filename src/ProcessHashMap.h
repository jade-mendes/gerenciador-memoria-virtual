//
// Created by Nathan Pinheiro on 05/06/2025.
//
#pragma once


#ifndef PROCESSHASHMAP_H
#define PROCESSHASHMAP_H
#include <stdint.h>

#include "nalloc.h"

typedef struct Process Process;


typedef struct ProcessHashMapEntry {
    uint32_t key;                       // PID
    Process* value;                     // Ponteiro para o processo
    struct ProcessHashMapEntry* next;   // Próxima entrada na lista encadeada
} ProcessHashMapEntry;

typedef struct ProcessHashMap {
    NallocContext nalloc_ctx;      // Contexto de alocação
    size_t capacity;                // Capacidade total (número de buckets)
    ProcessHashMapEntry** buckets;  // Array de buckets
} ProcessHashMap;

// Cria um novo hashmap
ProcessHashMap* process_hashmap_create(NallocContext ctx, size_t capacity);

// Insere ou atualiza um processo no hashmap
bool process_hashmap_put(ProcessHashMap* map, uint32_t pid, Process* process);

// Obtém um processo do hashmap pelo PID
bool process_hashmap_get(ProcessHashMap* map, uint32_t pid, Process** out_process);

// Remove um processo do hashmap pelo PID
bool process_hashmap_remove(ProcessHashMap* map, uint32_t pid);

// Destroi o hashmap e libera toda a memória
void process_hashmap_destroy(ProcessHashMap* map);


// Macro para iteração simples (não segura para remoção durante a iteração)
#define PROCESS_HASHMAP_FOREACH(map, entry_var) \
    for (size_t _bucket_idx_ = 0; _bucket_idx_ < (map)->capacity; _bucket_idx_++) \
        for (ProcessHashMapEntry* entry_var = (map)->buckets[_bucket_idx_]; \
             entry_var != NULL; \
             entry_var = entry_var->next)

// Macro para iteração segura (permite remoção durante a iteração)
#define PROCESS_HASHMAP_FOREACH_SAFE(map, entry_var, next_var) \
    for (size_t _bucket_idx_ = 0; _bucket_idx_ < (map)->capacity; _bucket_idx_++) \
        for (ProcessHashMapEntry *entry_var = (map)->buckets[_bucket_idx_], \
                                 *next_var = (entry_var ? entry_var->next : NULL); \
             entry_var != NULL; \
             entry_var = next_var, \
             next_var = (entry_var ? entry_var->next : NULL))

#endif //PROCESSHASHMAP_H
