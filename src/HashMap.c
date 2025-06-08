//
// Created by Nathan Pinheiro on 03/06/2025.
//

#include "HashMap.h"

#include <stdint.h>
#include <stdio.h>

#include "nalloc.h"
#include <string.h>

#define KEY_SIZE 8

// Função de hash melhorada para chaves de 8 bytes
static size_t hash_func(const char* key, const size_t capacity) {
    const size_t len = strlen(key);
    uint32_t hash = 0;
    // Processa até KEY_SIZE bytes ou até '\0'
    for (int i = 0; i < len; i++) {
        hash += key[i];
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
bool hashmap_put(HashMap* map, const char* key, int value) {
    //printf("\n ================ hashmap_put: key = %.8s, value = %d, \n", key, value);


    size_t index = hash_func(key, map->capacity);
    //printf("capacity = %zu, index = %zu ===============\n", map->capacity, index);

    HashMapEntry* current = map->buckets[index];

    // Verificação de chave existente
    while (current) {
        if (strncmp(current->key, key, KEY_SIZE) == 0) {
            current->value = value;
            return true;
        }
        current = current->next;
    }

    // Cria nova entrada
    HashMapEntry* new_entry = nalloc_alloc(map->nalloc_ctx, sizeof(HashMapEntry));
    if (!new_entry) return false;

    memcpy(new_entry->key, key, KEY_SIZE); // Copia dados do ponteiro
    new_entry->value = value;
    new_entry->next = map->buckets[index];
    map->buckets[index] = new_entry;
    return true;
}


// Obtém um valor do hashmap
bool hashmap_get(HashMap* map, const char* key, int* out_value) {
    size_t index = hash_func(key, map->capacity);
    HashMapEntry* current = map->buckets[index];

    while (current) {
        if (strncmp(current->key, key, KEY_SIZE) == 0) {
            *out_value = current->value;
            return true;
        }
        current = current->next;
    }
    return false;
}

// Remove uma entrada do hashmap
bool hashmap_remove(HashMap* map, const char* key) {
    size_t index = hash_func(key, map->capacity);
    HashMapEntry* current = map->buckets[index];
    HashMapEntry* prev = NULL;

    while (current) {
        if (strncmp(current->key, key, KEY_SIZE) == 0) {
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
    if (!map) return;

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

void print_hashmap(const HashMap* map) {
    for (size_t i = 0; i < map->capacity; i++) {
        HashMapEntry* current = map->buckets[i];
        if (current) {
            printf("Bucket %zu: ", i);
            while (current) {
                printf("{key: %.8s, value: %d} -> ", current->key, current->value);
                current = current->next;
            }
            printf("NULL\n");
        }
    }
}

HashMap* hashmap_clone(NallocContext* dest_ctx, const HashMap* src) {
    // Cria um novo hashmap com a mesma capacidade
    HashMap* new_map = hashmap_create(dest_ctx, src->capacity);
    if (!new_map) return NULL;

    // Clona todas as entradas
    for (size_t i = 0; i < src->capacity; i++) {
        // Clona a lista encadeada do bucket atual (mantendo ordem inversa)
        HashMapEntry* new_list = NULL;
        const HashMapEntry* current = src->buckets[i];

        while (current) {
            // Aloca nova entrada
            HashMapEntry* new_entry = nalloc_alloc(dest_ctx, sizeof(HashMapEntry));
            if (!new_entry) {
                // Falha: limpar entradas já alocadas neste bucket
                while (new_list) {
                    HashMapEntry* temp = new_list;
                    new_list = new_list->next;
                    nalloc_free(dest_ctx, temp);
                }
                hashmap_destroy(new_map);
                return NULL;
            }

            // Copia dados
            memcpy(new_entry->key, current->key, KEY_SIZE);
            new_entry->value = current->value;

            // Insere no início da lista (mantém ordem inversa)
            new_entry->next = new_list;
            new_list = new_entry;

            current = current->next;
        }

        // Atualiza o bucket do novo mapa
        new_map->buckets[i] = new_list;
    }

    return new_map;
}
