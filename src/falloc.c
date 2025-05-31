//
// Created by nathan on 12/26/24.
//


#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct nalloc_header {
    size_t size;
} BlockHeader;

typedef struct free_list {
    size_t size;
    struct free_list *next;
} FreeList;

typedef struct nalloc_context {
    void* addr;
    size_t total_size;
    FreeList* free_list;
} NallocContext;

NallocContext nalloc_new(void* pointer, const size_t size) {
    NallocContext nalloc_ctx;

    nalloc_ctx.addr = pointer;
    nalloc_ctx.total_size = size;

    FreeList* free_list = pointer;
    free_list->size = size - sizeof(FreeList);
    free_list->next = NULL;
    nalloc_ctx.free_list = free_list;

    return nalloc_ctx;
}

void* nalloc(NallocContext* f, const size_t size) {
    if (size == 0) return NULL;
    if (f->free_list == NULL) return NULL;

    FreeList** current = &f->free_list;

    while (*current != NULL) {
        if ((*current)->size >= size + sizeof(BlockHeader)) {
            FreeList* new = (FreeList*) ((char*)*current + size + sizeof(BlockHeader));
            new->size = (*current)->size - size;
            new->next = (*current)->next;

            BlockHeader* block = (BlockHeader*) *current;
            block->size = size;
            //block->is_free = false;

            *current = new;

            return block + 1; // Return pointer to the memory after the header
        }
        current = &(*current)->next;
    }

    return NULL;
}


void nalloc_free(NallocContext* f, void* ptr) {
    if (!ptr) return;
    if ((uint8_t*)ptr < (uint8_t*)f->addr ||
        (uint8_t*)ptr >= (uint8_t*)f->addr + f->total_size) {
        return; // Ponteiro fora dos limites
        }

    BlockHeader* header = (BlockHeader*)((char*)ptr - sizeof(BlockHeader));
    const size_t size = header->size;

    FreeList* new_block = (FreeList*)header;
    new_block->size = size + sizeof(BlockHeader);
    new_block->next = NULL;

    // Caso especial: lista vazia
    if (f->free_list == NULL) {
        f->free_list = new_block;
        return;
    }

    FreeList* prev = NULL;
    FreeList* curr = f->free_list;

    // Encontrar posição de inserção (ordenado por endereço)
    while (curr != NULL && (void*)curr < (void*)new_block) {
        prev = curr;
        curr = curr->next;
    }

    // Verificar fusão com o bloco anterior
    if (prev != NULL && (char*)prev + prev->size == (char*)new_block) {
        prev->size += new_block->size;
        // Usaremos 'prev' como base para fusões posteriores
    } else {
        // Inserir novo bloco entre prev e curr
        if (prev) {
            prev->next = new_block;
        } else {
            f->free_list = new_block;
        }
        new_block->next = curr;
        // Usaremos 'new_block' como base para fusões posteriores
        prev = new_block;
    }

    // Determinar bloco base para fusão
    FreeList* base = prev;

    // Fundir com blocos seguintes consecutivos
    FreeList* next_block = base->next;
    while (next_block != NULL && (char*)base + base->size == (char*)next_block) {
        base->size += next_block->size;
        base->next = next_block->next;
        next_block = base->next;
    }
}

void* nalloc_realloc(NallocContext* f, void* ptr, const size_t size) {
    if (!ptr) return nalloc(f, size);
    if (size == 0) {
        nalloc_free(f, ptr);
        return NULL;
    }
    BlockHeader* header = (BlockHeader*)((char*)ptr - sizeof(BlockHeader));
    if (header->size >= size) {
        return ptr;
    }
    void* new_ptr = nalloc(f, size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, header->size < size ? header->size : size);
        nalloc_free(f, ptr);
    }
    return new_ptr;
}

// Return the amount of memory allocated
uint32_t nalloc_allocated_size(NallocContext* f) {
    uint32_t livre = 0;
    const FreeList* current = f->free_list;

    while (current != NULL) {
        livre += current->size + sizeof(FreeList);
        current = current->next;
    }

    return f->total_size - livre;
}
uint32_t nalloc_total_size(NallocContext* f) {
    return f->total_size;
}


// void print_memory(NallocContext* f) {
//     const int max_width = 100;
//     char visualization[max_width + 1]; // +1 para o null terminator
//     memset(visualization, 0, sizeof(visualization));
//
//     uint8_t* start = (uint8_t*)f->addr;
//     uint8_t* end = start + f->total_size;
//     size_t total_bytes = f->total_size;
//     size_t bytes_per_char = (total_bytes + max_width - 1) / max_width; // Arredondar para cima
//
//     FreeList* current_free = f->free_list;
//     uint8_t* current = start;
//     int char_index = 0;
//
//     while (current < end && char_index < max_width) {
//         size_t segment_start = current - start;
//         size_t segment_end = segment_start + bytes_per_char;
//         if (segment_end > total_bytes) segment_end = total_bytes;
//
//         // Contar bytes livres neste segmento
//         size_t free_bytes = 0;
//         uint8_t* segment_ptr = current;
//
//         while (segment_ptr < start + segment_end) {
//             if (current_free && (void*)segment_ptr == (void*)current_free) {
//                 // Bloco livre encontrado
//                 size_t block_remaining = current_free->size - (segment_ptr - (uint8_t*)current_free);
//                 size_t segment_remaining = (start + segment_end) - segment_ptr;
//                 size_t bytes = block_remaining < segment_remaining ? block_remaining : segment_remaining;
//
//                 free_bytes += bytes;
//                 segment_ptr += bytes;
//
//                 // Avançar para o próximo bloco livre
//                 current_free = current_free->next;
//             } else {
//                 // Bloco alocado
//                 BlockHeader* header = (BlockHeader*)segment_ptr;
//                 size_t block_size = sizeof(BlockHeader) + header->size;
//                 size_t block_remaining = block_size - (segment_ptr - (uint8_t*)header);
//                 size_t segment_remaining = (start + segment_end) - segment_ptr;
//                 size_t bytes = block_remaining < segment_remaining ? block_remaining : segment_remaining;
//
//                 segment_ptr += bytes;
//             }
//         }
//
//         // Determinar a cor baseado na maioria dos bytes no segmento
//         size_t allocated_bytes = (segment_end - segment_start) - free_bytes;
//         if (free_bytes > allocated_bytes) {
//             visualization[char_index] = ' '; // Espaço verde para livre
//         } else {
//             visualization[char_index] = ' '; // Espaço vermelho para alocado
//         }
//
//         current = start + segment_end;
//         char_index++;
//     }
//
//     // Imprimir a visualização com cores
//     printf("[");
//     for (int i = 0; i < char_index; i++) {
//         if (visualization[i] == ' ') {
//             // Verificar se realmente representa mais espaço livre que alocado
//             // (isso é aproximado devido à compactação)
//             printf("\033[42m \033[0m"); // Verde para livre
//         } else {
//             printf("\033[41m \033[0m"); // Vermelho para alocado
//         }
//     }
//     printf("]\n");
// }



// write test
#include <assert.h>


int free_list_has_loop(NallocContext* ctx) {
    FreeList* head = ctx->free_list;
    if (head == NULL)
        return 0;  // Lista vazia

    FreeList* slow = head;
    FreeList* fast = head;
    int count = 0;
    const int max_nodes = 1000;  // Limite de segurança

    while (count < max_nodes) {
        // Verificação de ponteiro slow
        if ((uint8_t*)slow < (uint8_t*)ctx->addr ||
            (uint8_t*)slow >= (uint8_t*)ctx->addr + ctx->total_size) {
            return 1;  // Ponteiro inválido
            }

        // Avanço slow (1 passo)
        slow = slow->next;
        if (slow == NULL) break;

        // Avanço fast (2 passos) com verificação segura
        if (fast && fast->next) {
            fast = fast->next->next;
            // Verificação de ponteiro fast
            if (fast && ((uint8_t*)fast < (uint8_t*)ctx->addr ||
                         (uint8_t*)fast >= (uint8_t*)ctx->addr + ctx->total_size)) {
                return 1;
                         }
        } else {
            fast = NULL;
        }

        // Condições de término
        if (fast == NULL) break;
        if (slow == fast) return 1;  // Loop detectado

        count++;
    }

    return (count >= max_nodes);  // Possível loop se excedeu o limite
}

void test_nalloc_lib() {
    printf("Iniciando testes com verificações de corrupção...\n");
    uint8_t buffer[2048];
    NallocContext ctx = nalloc_new(buffer, sizeof(buffer));

    // 1. Verificação inicial
    printf("Verificando lista livre inicial...(%d) \n", free_list_has_loop(&ctx));
    assert(!free_list_has_loop(&ctx));

    // 2. Alocação básica
    void* ptr1 = nalloc(&ctx, 64);
    assert(ptr1);
    assert(!free_list_has_loop(&ctx));

    // 3. Alocação múltipla
    void* ptrs[5];
    for (int i = 0; i < 5; i++) {
        ptrs[i] = nalloc(&ctx, 32);
        assert(ptrs[i]);
        assert(!free_list_has_loop(&ctx));
    }

    // 4. Liberação seletiva
    for (int i = 0; i < 5; i += 2) {
        nalloc_free(&ctx, ptrs[i]);
        assert(!free_list_has_loop(&ctx));
    }

    // 5. Teste de realocação
    void* rptr = nalloc_realloc(&ctx, ptrs[1], 128);
    assert(rptr);
    assert(!free_list_has_loop(&ctx));

    // 6. Teste de fragmentação intensa
    void* small[20];
    for (int i = 0; i < 20; i++) {
        small[i] = nalloc(&ctx, 8);
        assert(small[i]);
    }
    assert(!free_list_has_loop(&ctx));

    // 7. Liberação em padrão de xadrez
    for (int i = 0; i < 20; i += 2) {
        nalloc_free(&ctx, small[i]);
    }
    assert(!free_list_has_loop(&ctx));

    // 8. Reutilização de espaços
    for (int i = 0; i < 10; i++) {
        assert(nalloc(&ctx, 8));
    }
    assert(!free_list_has_loop(&ctx));

    // 9. Teste de estresse final
    // size_t remaining = ctx.free_list->size;
    // void* last = nalloc(&ctx, remaining - sizeof(BlockHeader) - 1);
    // assert(last);
    // assert(ctx.free_list == NULL);  // Memória esgotada

    // 10. Liberação total
    // nalloc_free(&ctx, last);
    for (int i = 1; i < 20; i += 2) {
        nalloc_free(&ctx, small[i]);
    }
    nalloc_free(&ctx, rptr);
    assert(!free_list_has_loop(&ctx));

    printf("Todos os testes passaram sem corrupção de memória!\n");
}


// Execute os testes no main
int main() {
    test_nalloc_lib();
    return 0;
}
