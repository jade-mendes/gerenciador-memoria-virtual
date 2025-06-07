//
// Created by Nathan Pinheiro on 03/06/2025.
//

#ifndef HASHMAP_H
#define HASHMAP_H

#include "nalloc.h"

#define KEY_SIZE 8

/// Estrutura principal do hashmap (opaca ao usuário)
typedef struct HashMap HashMap;

/// Cria um novo hashmap com a capacidade especificada
/// Retorna NULL em caso de falha
HashMap* hashmap_create(NallocContext* ctx, size_t capacity);

/// Insere ou atualiza um valor no hashmap
/// Retorna true em caso de sucesso, false em caso de falha
bool hashmap_put(HashMap* map, const char key[KEY_SIZE], int value);

/// Obtém um valor do hashmap
/// Retorna true se a chave foi encontrada, false caso contrário
/// O valor é armazenado em out_value se a chave for encontrada
bool hashmap_get(HashMap* map, const char key[KEY_SIZE], int* out_value);

/// Remove uma entrada do hashmap
/// Retorna true se a chave foi encontrada e removida, false caso contrário
bool hashmap_remove(HashMap* map, const char key[KEY_SIZE]);

/// Destroi o hashmap e libera toda a memória associada
void hashmap_destroy(HashMap* map);

#endif // HASHMAP_H
