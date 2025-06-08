//
// Created by Nathan Pinheiro on 05/06/2025.
//


#include "ProcessHashMap.h"
#include "nalloc.h"
#include <string.h>

// Função de hash otimizada para PIDs
static size_t pid_hash_func(uint32_t key, size_t capacity) {
    return (key * 2654435761U) % capacity; // Multiplicação por primo grande
}

ProcessHashMap* process_hashmap_create(NallocContext ctx, size_t capacity) {
    ProcessHashMap* map = nalloc_alloc(&ctx, sizeof(ProcessHashMap));
    if (!map) return NULL;

    map->nalloc_ctx = ctx;
    map->capacity = capacity;
    map->buckets = nalloc_alloc(&ctx, capacity * sizeof(ProcessHashMapEntry*));

    if (!map->buckets) {
        nalloc_free(&ctx, map);
        return NULL;
    }

    memset(map->buckets, 0, capacity * sizeof(ProcessHashMapEntry*));
    return map;
}

bool process_hashmap_put(ProcessHashMap* map, uint32_t pid, Process* process) {
    size_t index = pid_hash_func(pid, map->capacity);
    ProcessHashMapEntry* current = map->buckets[index];

    // Verifica se o PID já existe
    while (current) {
        if (current->key == pid) {
            current->value = process; // Atualiza o processo existente
            return true;
        }
        current = current->next;
    }

    // Cria nova entrada
    ProcessHashMapEntry* new_entry = nalloc_alloc(&map->nalloc_ctx, sizeof(ProcessHashMapEntry));
    if (!new_entry) return false;

    new_entry->key = pid;
    new_entry->value = process;
    new_entry->next = map->buckets[index]; // Insere no início do bucket
    map->buckets[index] = new_entry;
    return true;
}

bool process_hashmap_get(ProcessHashMap* map, uint32_t pid, Process** out_process) {
    size_t index = pid_hash_func(pid, map->capacity);
    ProcessHashMapEntry* current = map->buckets[index];

    while (current) {
        if (current->key == pid) {
            *out_process = current->value;
            return true;
        }
        current = current->next;
    }
    return false;
}

bool process_hashmap_is_empty(ProcessHashMap* map) {
    if (!map || !map->buckets) {
        return true; // Mapa não inicializado ou sem buckets
    }
    for (size_t i = 0; i < map->capacity; i++) {
        if (map->buckets[i]) {
            return false; // Pelo menos um bucket não está vazio
        }
    }
    return true; // Todos os buckets estão vazios
}

bool process_hashmap_remove(ProcessHashMap* map, uint32_t pid) {
    size_t index = pid_hash_func(pid, map->capacity);
    ProcessHashMapEntry* current = map->buckets[index];
    ProcessHashMapEntry* prev = NULL;

    while (current) {
        if (current->key == pid) {
            if (prev) {
                prev->next = current->next;
            } else {
                map->buckets[index] = current->next;
            }
            nalloc_free(&map->nalloc_ctx, current);
            return true;
        }
        prev = current;
        current = current->next;
    }
    return false;
}

void process_hashmap_destroy(ProcessHashMap* map) {
    for (size_t i = 0; i < map->capacity; i++) {
        ProcessHashMapEntry* current = map->buckets[i];
        while (current) {
            ProcessHashMapEntry* next = current->next;
            nalloc_free(&map->nalloc_ctx, current);
            current = next;
        }
    }
    nalloc_free(&map->nalloc_ctx, map->buckets);
    nalloc_free(&map->nalloc_ctx, map);
}