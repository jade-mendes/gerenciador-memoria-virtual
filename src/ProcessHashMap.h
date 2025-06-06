//
// Created by Nathan Pinheiro on 05/06/2025.
//
#pragma once


#ifndef PROCESSHASHMAP_H
#include "process.h"

typedef struct ProcessHashMapEntry {
    uint32_t key;                       // PID
    Process* value;                     // Ponteiro para o processo
    struct ProcessHashMapEntry* next;   // Próxima entrada na lista encadeada
} ProcessHashMapEntry;

typedef struct ProcessHashMap {
    NallocContext* nalloc_ctx;      // Contexto de alocação
    size_t capacity;                // Capacidade total (número de buckets)
    ProcessHashMapEntry** buckets;  // Array de buckets
} ProcessHashMap;

// Cria um novo hashmap
ProcessHashMap* process_hashmap_create(NallocContext* ctx, size_t capacity);

// Insere ou atualiza um processo no hashmap
bool process_hashmap_put(ProcessHashMap* map, uint32_t pid, Process* process);

// Obtém um processo do hashmap pelo PID
bool process_hashmap_get(ProcessHashMap* map, uint32_t pid, Process** out_process);

// Remove um processo do hashmap pelo PID
bool process_hashmap_remove(ProcessHashMap* map, uint32_t pid);

// Destroi o hashmap e libera toda a memória
void process_hashmap_destroy(ProcessHashMap* map);


#define PROCESSHASHMAP_H

#endif //PROCESSHASHMAP_H
