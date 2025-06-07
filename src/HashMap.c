//
// Created by Nathan Pinheiro on 03/06/2025.
//

#include "nalloc.h"
#include <string.h>

#define KEY_SIZE 8

// Estrutura de uma entrada no hashmap
typedef struct HashMapEntry {
    char key[KEY_SIZE];
    int value;
    struct HashMapEntry* next;
} HashMapEntry;

// Estrutura principal do hashmap
typedef struct {
    NallocContext* nalloc_ctx; // Contexto de alocação
    size_t capacity;            // Capacidade total do hashmap
    HashMapEntry** buckets;     // Array de baldes (buckets)
} HashMap;

// Função de hash simples para chaves de 8 bytes
static size_t hash_func(const char key[KEY_SIZE], size_t capacity) {
    size_t hash = 0;
    for (int i = 0; i < KEY_SIZE; i++) {
        hash = (hash << 5) + key[i];
    }
    return hash % capacity;
}

// Cria um novo hashmap
HashMap* hashmap_create(NallocContext* ctx, size_t capacity) {
    // Aloca a estrutura principal
    HashMap* map = nalloc_alloc(ctx, sizeof(HashMap));
    if (!map) return NULL;

    map->nalloc_ctx = ctx;
    map->capacity = capacity;

    // Aloca o array de buckets
    map->buckets = nalloc_alloc(ctx, capacity * sizeof(HashMapEntry*));
    if (!map->buckets) {
        nalloc_free(ctx, map);
        return NULL;
    }

    // Inicializa todos os buckets como NULL
    memset(map->buckets, 0, capacity * sizeof(HashMapEntry*));
    return map;
}

// Insere ou atualiza um valor no hashmap
bool hashmap_put(HashMap* map, const char key[KEY_SIZE], int value) {
    size_t index = hash_func(key, map->capacity);
    HashMapEntry* current = map->buckets[index];

    // Verifica se a chave já existe
    while (current) {
        if (memcmp(current->key, key, KEY_SIZE) == 0) {
            current->value = value; // Atualiza o valor
            return true;
        }
        current = current->next;
    }

    // Cria nova entrada
    HashMapEntry* new_entry = nalloc_alloc(map->nalloc_ctx, sizeof(HashMapEntry));
    if (!new_entry) return false;

    memcpy(new_entry->key, key, KEY_SIZE);
    new_entry->value = value;
    new_entry->next = map->buckets[index]; // Insere no início
    map->buckets[index] = new_entry;
    return true;
}

// Obtém um valor do hashmap
bool hashmap_get(HashMap* map, const char key[KEY_SIZE], int* out_value) {
    size_t index = hash_func(key, map->capacity);
    HashMapEntry* current = map->buckets[index];

    while (current) {
        if (memcmp(current->key, key, KEY_SIZE) == 0) {
            *out_value = current->value;
            return true;
        }
        current = current->next;
    }
    return false;
}

// Remove uma entrada do hashmap
bool hashmap_remove(HashMap* map, const char key[KEY_SIZE]) {
    size_t index = hash_func(key, map->capacity);
    HashMapEntry* current = map->buckets[index];
    HashMapEntry* prev = NULL;

    while (current) {
        if (memcmp(current->key, key, KEY_SIZE) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                map->buckets[index] = current->next;
            }
            nalloc_free(map->nalloc_ctx, current);
            return true;
        }
        prev = current;
        current = current->next;
    }
    return false;
}

// Destroi o hashmap e libera toda a memória
void hashmap_destroy(HashMap* map) {
    for (size_t i = 0; i < map->capacity; i++) {
        HashMapEntry* current = map->buckets[i];
        while (current) {
            HashMapEntry* next = current->next;
            nalloc_free(map->nalloc_ctx, current);
            current = next;
        }
    }
    nalloc_free(map->nalloc_ctx, map->buckets);
    nalloc_free(map->nalloc_ctx, map);
}