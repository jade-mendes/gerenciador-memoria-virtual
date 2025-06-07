#ifndef NALLOC_H
#define NALLOC_H

#include <stdbool.h>
#include <stddef.h>

// Estrutura do cabeçalho de bloco de memória
typedef struct block_header {
    size_t size;        // Tamanho do bloco de dados (excluindo o cabeçalho)
    bool is_free;       // Flag indicando se o bloco está livre
} BlockHeader;

// Contexto do alocador
typedef struct nalloc_context {
    void* base_addr;    // Endereço base da área de memória gerenciada
    size_t total_size;  // Tamanho total da área gerenciada
} NallocContext;

// Interface pública da biblioteca

// Inicializa um novo contexto de alocação
NallocContext nalloc_init(void* buffer, size_t size);

// Aloca memória no contexto especificado
void* nalloc_alloc(const NallocContext* ctx, size_t size);

// Libera memória no contexto especificado
void nalloc_free(const NallocContext* ctx, void* ptr);

// Realoca memória no contexto especificado
void* nalloc_realloc(const NallocContext* ctx, void* ptr, size_t size);

// Calcula o total de memória alocada no contexto
size_t nalloc_allocated_size(NallocContext* ctx);

// Calcula o total de memória livre disponível no contexto
size_t nalloc_free_size(NallocContext* ctx);

// Função de visualização da memória (para debugging)
void nalloc_print_memory(NallocContext* ctx);

#endif // NALLOC_H