#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct block_header {
    size_t size;
    bool is_free;
} BlockHeader;

typedef struct nalloc_context {
    void* base_addr;
    size_t total_size;
} NallocContext;

// Inicializa um novo contexto de alocação
NallocContext nalloc_init(void* buffer, const size_t size) {
    NallocContext ctx;
    ctx.base_addr = buffer;
    ctx.total_size = size;

    // Inicializa o primeiro bloco como livre
    BlockHeader* header = buffer;
    header->size = size - sizeof(BlockHeader);
    header->is_free = true;
    return ctx;
}

// Aloca memória no contexto especificado
void* nalloc_alloc(NallocContext* ctx, size_t size) {
    void* current = ctx->base_addr;
    const void* end = (char*)ctx->base_addr + ctx->total_size;

    while (current < end) {
        BlockHeader* header = (BlockHeader*)current;

        if (header->is_free && header->size >= size) {
            // Verifica se pode dividir o bloco
            if (header->size >= size + sizeof(BlockHeader) + 1) {
                BlockHeader* new_block = (BlockHeader*)((char*)current + sizeof(BlockHeader) + size);
                new_block->size = header->size - size - sizeof(BlockHeader);
                new_block->is_free = true;

                header->size = size;
            }
            header->is_free = false;
            return (char*)current + sizeof(BlockHeader);
        }

        // Avança para o próximo bloco
        current = (char*)current + sizeof(BlockHeader) + header->size;
    }

    return NULL; // Não há espaço disponível
}

// Libera memória no contexto especificado
void nalloc_free(NallocContext* ctx, void* ptr) {
    if (!ptr) return;

    BlockHeader* header = (BlockHeader*)((char*)ptr - sizeof(BlockHeader));
    header->is_free = true;

    // Coalescência com o próximo bloco (se livre)
    BlockHeader* next = (BlockHeader*)((char*)header + sizeof(BlockHeader) + header->size);
    if ((char*)next < (char*)ctx->base_addr + ctx->total_size && next->is_free) {
        header->size += sizeof(BlockHeader) + next->size;
    }
}

// Realoca memória no contexto especificado
void* nalloc_realloc(NallocContext* ctx, void* ptr, size_t size) {
    if (!ptr) return nalloc_alloc(ctx, size);

    BlockHeader* header = (BlockHeader*)((char*)ptr - sizeof(BlockHeader));
    if (header->size >= size) {
        return ptr; // Bloco atual já é suficiente
    }

    void* new_ptr = nalloc_alloc(ctx, size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, header->size);
        nalloc_free(ctx, ptr);
    }
    return new_ptr;
}

// Calcula o total de memória alocada no contexto
size_t nalloc_allocated_size(NallocContext* ctx) {
    size_t total = 0;
    void* current = ctx->base_addr;
    const void* end = (char*)ctx->base_addr + ctx->total_size;

    while (current < end) {
        BlockHeader* header = (BlockHeader*)current;
        if (!header->is_free) {
            total += header->size;
        }
        current = (char*)current + sizeof(BlockHeader) + header->size;
    }
    return total;
}

// Calcula o total de memória livre disponível no contexto
size_t nalloc_free_size(NallocContext* ctx) {
    size_t total = 0;
    void* current = ctx->base_addr;
    const void* end = (char*)ctx->base_addr + ctx->total_size;

    while (current < end) {
        BlockHeader* header = (BlockHeader*) current;
        if (header->is_free) {
            total += header->size;
        }
        current = (char*)current + sizeof(BlockHeader) + header->size;
    }
    return total;
}

void nalloc_print_memory(NallocContext* ctx) {
    const int bar_width = 150; // Largura máxima da barra
    char visual[bar_width + 1];
    visual[bar_width] = '\0'; // Terminador de string

    // Calcula o tamanho de cada unidade visual
    const double unit_size = (double)ctx->total_size / bar_width;

    void* current = ctx->base_addr;
    const void* end = (char*)ctx->base_addr + ctx->total_size;
    int bar_index = 0;
    double accumulated = 0.0;

    while (current < end && bar_index < bar_width) {
        BlockHeader* header = (BlockHeader*)current;
        size_t block_size = sizeof(BlockHeader) + header->size;
        double block_units = block_size / unit_size;

        // Preenche a representação visual para este bloco
        while (block_units > 0 && bar_index < bar_width) {
            double fill_amount = (block_units > 1.0) ? 1.0 : block_units;
            block_units -= fill_amount;
            accumulated += fill_amount;

            if (accumulated >= 0.95) { // Threshold para preenchimento completo
                visual[bar_index] = header->is_free ? 'F' : 'A';
                accumulated = 0.0;
                bar_index++;
            }
        }

        current = (char*)current + block_size;
    }

    // Preenche o restante (se houver)
    for (; bar_index < bar_width; bar_index++) {
        visual[bar_index] = '?';
    }

    // Imprime a barra colorida
    printf("[");
    for (int i = 0; i < bar_width; i++) {
        switch (visual[i]) {
            case 'F': // Bloco livre: verde
                printf("\033[1;32m▓\033[0m");
                break;
            case 'A': // Bloco alocado: vermelho
                printf("\033[1;31m▓\033[0m");
                break;
            default: // Não mapeado: cinza
                printf("\033[1;37m░\033[0m");
        }
    }
    printf("]\n");
}


// int main() {
//     const size_t BUFFER_SIZE = 1024; // 1KB para testes
//     char buffer[BUFFER_SIZE];
//
//     // 1. Inicialização do alocador
//     NallocContext ctx = nalloc_init(buffer, BUFFER_SIZE);
//
//     printf("=== Estado Inicial ===\n");
//     nalloc_print_memory(&ctx);
//     printf("Memória total: %zu bytes\n", BUFFER_SIZE);
//     printf("Livre inicial: %zu bytes\n", nalloc_free_size(&ctx));
//     printf("Alocada inicial: %zu bytes\n\n", nalloc_allocated_size(&ctx));
//
//     // 2. Teste de alocação básica
//     char *ptr1 = nalloc_alloc(&ctx, 100);
//     char *ptr2 = nalloc_alloc(&ctx, 200);
//     printf("=== Após duas alocações ===\n");
//     nalloc_print_memory(&ctx);
//     printf("ptr1: %p [100 bytes]\n", (void*)ptr1);
//     printf("ptr2: %p [200 bytes]\n", (void*)ptr2);
//     printf("Livre: %zu bytes\n", nalloc_free_size(&ctx));
//     printf("Alocada: %zu bytes\n\n", nalloc_allocated_size(&ctx));
//
//     // 3. Teste de liberação e coalescência
//     nalloc_free(&ctx, ptr1);
//     printf("=== Após liberar ptr1 ===\n");
//     nalloc_print_memory(&ctx);
//     printf("Livre: %zu bytes (deve aumentar 100 + header)\n", nalloc_free_size(&ctx));
//     printf("Alocada: %zu bytes\n\n", nalloc_allocated_size(&ctx));
//
//     // 4. Teste de realocação (crescimento)
//     char *new_ptr2 = nalloc_realloc(&ctx, ptr2, 300);
//     printf("=== Após realocar ptr2 (200->300) ===\n");
//     nalloc_print_memory(&ctx);
//     printf("Original: %p\n", (void*)ptr2);
//     printf("Novo: %p %s\n", (void*)new_ptr2,
//           (ptr2 != new_ptr2) ? "(movido)" : "(mesmo local)");
//     printf("Livre: %zu bytes\n", nalloc_free_size(&ctx));
//     printf("Alocada: %zu bytes\n\n", nalloc_allocated_size(&ctx));
//
//     // 5. Teste de fragmentação
//     char *ptr3 = nalloc_alloc(&ctx, 150);
//     char *ptr4 = nalloc_alloc(&ctx, 100);
//     printf("=== Após alocar ptr3 e ptr4 ===\n");
//     nalloc_print_memory(&ctx);
//     printf("ptr3: %p [150 bytes]\n", (void*)ptr3);
//     printf("ptr4: %p [100 bytes]\n", (void*)ptr4);
//     printf("Livre: %zu bytes\n", nalloc_free_size(&ctx));
//     printf("Alocada: %zu bytes\n\n", nalloc_allocated_size(&ctx));
//
//     // 6. Teste de estouro de capacidade
//     char *ptr5 = nalloc_alloc(&ctx, 1000);
//     printf("=== Tentativa de alocação grande ===\n");
//     nalloc_print_memory(&ctx);
//     printf("ptr5: %p %s\n\n", (void*)ptr5,
//           ptr5 ? "(sucesso)" : "(falha esperada)");
//
//     // 7. Liberação total e verificação final
//     nalloc_free(&ctx, new_ptr2);
//     nalloc_free(&ctx, ptr3);
//     nalloc_free(&ctx, ptr4);
//     printf("=== Após liberar tudo ===\n");
//     nalloc_print_memory(&ctx);
//     printf("Livre: %zu bytes\n", nalloc_free_size(&ctx));
//     printf("Alocada: %zu bytes\n", nalloc_allocated_size(&ctx));
//
//     return 0;
// }