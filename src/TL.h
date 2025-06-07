///
/// Created by nathan on 02/10/2024.
///

#ifndef TYPE_LIST_H
#define TYPE_LIST_H

#include <stdlib.h>

/// @brief Estrutura de lista dinâmica genérica.
typedef struct type_list {
    size_t len;        ///< Quantidade de elementos na lista.
    size_t cap;        ///< Capacidade atual da lista.
    size_t data_size;  ///< Tamanho (em bytes) de cada elemento.
    char* arr;         ///< Ponteiro para os dados.
} TL;

/// @brief Cria uma nova lista dinâmica genérica.
///
/// @param data_size Tamanho, em bytes, do tipo que será armazenado.
/// @return Uma nova lista dinâmica inicializada.
TL TL_new(const size_t data_size);

/// @brief Libera a memória alocada pela lista.
///
/// @param tl Ponteiro constante para a lista a ser liberada.
/// @note A própria struct não é liberada, apenas os dados internos.
void TL_free(const TL* tl);

/// @brief Adiciona um elemento no final da lista.
///
/// @param tl Ponteiro para a lista.
/// @param data Ponteiro para os dados a serem adicionados.
void TL_push(TL* tl, const void* data);

/// @brief Remove e retorna o ponteiro para o último elemento da lista.
///
/// @param tl Ponteiro para a lista.
/// @return Ponteiro para os dados do elemento removido dentro do array interno.
/// @warning Esse ponteiro é interno; não deve ser liberado manualmente.
void* TL_pop(TL* tl);

/// @brief Retorna um ponteiro para o elemento na posição especificada.
///
/// @param tl Ponteiro constante para a lista.
/// @param index Índice do elemento (0-baseado).
/// @return Ponteiro para os dados do elemento.
/// @warning Esse ponteiro é interno; não deve ser liberado manualmente.
void* TL_get(const TL* tl, const size_t index);

/// @brief Substitui o elemento na posição especificada pelos dados fornecidos.
///
/// @param tl Ponteiro para a lista.
/// @param index Índice do elemento (0-baseado) que será sobrescrito.
/// @param data Ponteiro para os novos dados.
void TL_set(TL* tl, const size_t index, const void* data);

#endif // TYPE_LIST_H
