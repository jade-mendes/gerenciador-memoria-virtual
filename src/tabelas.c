//
// Created by nathan on 08/06/2025.
//

#include "tabelas.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "process.h"
#include "Simulador.h"

// Funções de gerenciamento da TLB
TLB* create_tlb(NallocContext* ctx, const uint32_t size) {
    TLB* tlb = nalloc_alloc(ctx, sizeof(TLB));
    if (!tlb) return NULL;

    tlb->size = size;
    tlb->entries = (TLB_ENTRY*)nalloc_alloc(ctx, size * sizeof(TLB_ENTRY));
    tlb->clock_hand = 0; // Inicializa ponteiro do CLOCK

    if (!tlb->entries) {
        nalloc_free(ctx, tlb);
        return NULL;
    }

    for (uint32_t i = 0; i < size; i++) {
        tlb->entries[i].valid = false;
        tlb->entries[i].referenced = false; // Inicializa bit de referência
    }

    return tlb;
}

void destroy_tlb(const NallocContext* ctx, TLB* tlb) {
    if (tlb->entries) {
        nalloc_free(ctx, tlb->entries);
    }
    nalloc_free(ctx, tlb);
}

bool tlb_lookup(TLB* tlb, const uint32_t page, uintptr_t* frame) {
    for (uint32_t i = 0; i < tlb->size; i++) {
        if (tlb->entries[i].valid && tlb->entries[i].page == page) {
            *frame = tlb->entries[i].frame;
            tlb->entries[i].last_used = simulador->current_cycle;
            tlb->entries[i].referenced = true; // Marca como referenciada
            return true;
        }
    }
    return false;
}

void tlb_update(TLB* tlb, const uint32_t page, const uintptr_t frame) {
    uint32_t index = 0;
    SubPolicyType policy = simulador->config.SUB_POLICY_TYPE;

    switch (policy) {
        case SUB_POLICY_LRU: {
            // Política LRU (Least Recently Used)
            uint64_t min_used = UINT64_MAX;
            for (uint32_t i = 0; i < tlb->size; i++) {
                if (!tlb->entries[i].valid) {
                    index = i;
                    break;
                }
                if (tlb->entries[i].last_used < min_used) {
                    min_used = tlb->entries[i].last_used;
                    index = i;
                }
            }
            break;
        }
        case SUB_POLICY_CLOCK: {
            // Política CLOCK (Segunda Chance)
            while (1) {
                index = tlb->clock_hand;
                tlb->clock_hand = (tlb->clock_hand + 1) % tlb->size;

                if (!tlb->entries[index].valid) break;

                if (tlb->entries[index].referenced) {
                    tlb->entries[index].referenced = false;
                } else {
                    break;
                }
            }
            break;
        }
        case SUB_POLICY_RANDOM: {
            // Política RANDOM (Aleatória)
            uint32_t invalid_count = 0;
            for (uint32_t i = 0; i < tlb->size; i++) {
                if (!tlb->entries[i].valid) {
                    index = i;
                    invalid_count++;
                }
            }

            // Se há entradas inválidas, usa a primeira encontrada
            // Caso contrário, escolhe aleatoriamente
            if (invalid_count == 0) {
                index = rand() % tlb->size;
            }
            break;
        }
    }

    // Atualiza a entrada selecionada
    tlb->entries[index].page = page;
    tlb->entries[index].frame = frame;
    tlb->entries[index].valid = true;
    tlb->entries[index].last_used = simulador->current_cycle;
    tlb->entries[index].referenced = true; // Marca como referenciada
}


void tlb_invalidate_entry(const TLB* tlb, const uint32_t page) {
    for (uint32_t i = 0; i < tlb->size; i++) {
        if (tlb->entries[i].valid && tlb->entries[i].page == page) {
            tlb->entries[i].valid = false;
            tlb->entries[i].referenced = false; // Reseta bit de referência
            break;
        }
    }
}

void reset_tlb_validity(const TLB* tlb) {
    printf("\nResetando TLB\n");
    for (uint32_t i = 0; i < tlb->size; i++) {
        tlb->entries[i].valid = false;
        tlb->entries[i].referenced = false; // Reseta bit de referência
    }
}

// Funções de acesso à memória
uint8_t get_mem(const Simulador* s, Process* p, const uint32_t virt_addr, bool use_tlb, int* out_status) {
    // Verificar se o endereço excede o espaço lógico
    const uint32_t max_virt_addr = (1U << s->config.BITS_LOGICAL_ADDRESS) - 1;
    if (virt_addr > max_virt_addr) {
        *out_status = MEM_ACCESS_INVALID_ADDRESS;
        return 0;
    }

    const uint32_t page_num = virt_addr / s->config.PAGE_SIZE;
    const uint32_t offset = virt_addr % s->config.PAGE_SIZE;

    uintptr_t frame_addr;
    bool tlb_hit = false;
    if (use_tlb) {
       tlb_hit = tlb_lookup(s->tlb, page_num, &frame_addr);
    }


    if (page_num >= p->page_table->num_entries) {
        *out_status = MEM_ACCESS_PAGE_NOT_ALLOCATED;
        return 0;
    }
    PAGE_TABLE_ENTRY* entry = &p->page_table->entries[page_num];

    if (!entry->valid) {
        *out_status = MEM_ACCESS_PAGE_NOT_ALLOCATED;
        return 0;
    }

    if (!tlb_hit) {
        frame_addr = entry->frame;
        if (use_tlb) {
            tlb_update(s->tlb, page_num, frame_addr);
        }
    }

    entry->referenced = true;

    const uint8_t* physical_addr = (uint8_t*)(frame_addr + offset);
    *out_status = MEM_ACCESS_OK;
    return *physical_addr;
}

void set_mem(const Simulador* s, Process* p, const uint32_t virt_addr, const uint8_t value, bool use_tlb, int* out_status) {
    // Verificar se o endereço excede o espaço lógico
    const uint32_t max_virt_addr = (1U << s->config.BITS_LOGICAL_ADDRESS) - 1;
    if (virt_addr > max_virt_addr) {
        *out_status = MEM_ACCESS_INVALID_ADDRESS;
        return;
    }

    // Calcular o número da página e o deslocamento
    const uint32_t page_num = virt_addr / s->config.PAGE_SIZE;
    const uint32_t offset = virt_addr % s->config.PAGE_SIZE;

    uintptr_t frame_addr;
    bool tlb_hit = false;
    if (use_tlb) {
        tlb_hit = tlb_lookup(s->tlb, page_num, &frame_addr);
    }

    if (page_num >= p->page_table->num_entries) {
        *out_status = MEM_ACCESS_PAGE_NOT_ALLOCATED;
        return;
    }
    PAGE_TABLE_ENTRY* entry = &p->page_table->entries[page_num];

    if (!entry->valid) {
        *out_status = MEM_ACCESS_PAGE_NOT_ALLOCATED;
        return;
    }

    if (!tlb_hit) {
        frame_addr = entry->frame;
        if (use_tlb) {
            tlb_update(s->tlb, page_num, frame_addr);
        }
    }

    entry->referenced = true;
    entry->dirty = true;

    uint8_t* physical_addr = (uint8_t*)(frame_addr + offset);
    *physical_addr = value;
    *out_status = MEM_ACCESS_OK;
}

// Funções de gerenciamento de páginas ==============

// Aloca uma página virtual para o processo
bool allocate_page(const Simulador* s, Process* p, uintptr_t virt_addr) {
    const uint32_t page_num = virt_addr / s->config.PAGE_SIZE;

    if (page_num < p->page_table->num_entries && p->page_table->entries[page_num].valid) {
        return true; // Página já alocada
    }

    const NallocContext* context = p->state == PROCESS_SUSPENDED ? &s->secondary_memory_ctx : &s->main_memory_ctx;

    const uintptr_t frame_addr = (uintptr_t) nalloc_alloc(context, s->config.PAGE_SIZE);
    if (!frame_addr) {
        return false; // Falha na alocação de memória
    }

    if (page_num >= p->page_table->num_entries) {
        const uint32_t new_num_entries = page_num + 1;
        PAGE_TABLE_ENTRY* new_entries = nalloc_realloc(
            context,
            p->page_table->entries,
            new_num_entries * sizeof(PAGE_TABLE_ENTRY)
        );

        if (!new_entries) {
            nalloc_free(context, (void*)frame_addr);
            return false; // Falha na realocação de memória
        }

        for (uint32_t i = p->page_table->num_entries; i < new_num_entries; i++) {
            new_entries[i].valid = false;
            new_entries[i].frame = INVALID_FRAME;
            new_entries[i].dirty = false;
            new_entries[i].referenced = false;
        }

        p->page_table->entries = new_entries;
        p->page_table->num_entries = new_num_entries;
    }

    p->page_table->entries[page_num].valid = true;
    p->page_table->entries[page_num].frame = frame_addr;
    p->page_table->entries[page_num].dirty = false;
    p->page_table->entries[page_num].referenced = false;

    return true;
}

// Libera uma página virtual do processo
// Pode ser chamada apenas por processos na memória principal
void deallocate_page(Simulador* s, Process* p, uint32_t virt_addr) {
    const uint32_t page_num = virt_addr / s->config.PAGE_SIZE;

    if (page_num >= p->page_table->num_entries || !p->page_table->entries[page_num].valid) {
        return;
    }

    const NallocContext* context = p->state == PROCESS_SUSPENDED ? &s->secondary_memory_ctx : &s->main_memory_ctx;

    void* frame_addr = (void*) p->page_table->entries[page_num].frame;

    nalloc_free(context, frame_addr);
    p->page_table->entries[page_num].valid = false;
    tlb_invalidate_entry(s->tlb, page_num);
}

// Funções de criação/destruição das estruturas
PAGE_TABLE* create_page_table(const NallocContext* ctx) {
    PAGE_TABLE* pt = nalloc_alloc(ctx, sizeof(PAGE_TABLE));
    if (!pt) return NULL;

    pt->num_entries = 0;
    pt->entries = NULL;
    return pt;
}

void destroy_page_table(const NallocContext* ctx, PAGE_TABLE* pt) {
    if (!pt) return;

    if (pt->entries) {
        for (uint32_t i = 0; i < pt->num_entries; i++) {
            if (pt->entries[i].valid) {
                // Libera o frame associado à página
                void* frame_addr = (void*) pt->entries[i].frame;
                if (frame_addr) {
                    nalloc_free(ctx, frame_addr);
                }
            }
        }

        nalloc_free(ctx, pt->entries);
    }

    nalloc_free(ctx, pt);
}

// Função para destruir páginas de um processo
void destroy_process_pages(Simulador* s, Process* p) {
    for (uint32_t i = 0; i < p->page_table->num_entries; i++) {
        if (p->page_table->entries[i].valid) {
            deallocate_page(s, p, i * s->config.PAGE_SIZE);
        }
    }
}


PAGE_TABLE* page_table_clone(const NallocContext* new_ctx, const PAGE_TABLE* original_pt, const uint32_t page_size) {
    // Alocar nova estrutura de tabela de páginas
    PAGE_TABLE* new_pt = nalloc_alloc(new_ctx, sizeof(PAGE_TABLE));
    if (!new_pt) return NULL;

    // Inicializar campos
    new_pt->num_entries = original_pt->num_entries;
    new_pt->entries = NULL;

    // Alocar array de entradas se necessário
    if (new_pt->num_entries > 0) {
        new_pt->entries = nalloc_alloc(new_ctx, new_pt->num_entries * sizeof(PAGE_TABLE_ENTRY));
        if (!new_pt->entries) {
            nalloc_free(new_ctx, new_pt);
            return NULL;
        }

        // Copiar cada entrada
        for (uint32_t i = 0; i < new_pt->num_entries; i++) {
            const PAGE_TABLE_ENTRY* orig_entry = &original_pt->entries[i];
            PAGE_TABLE_ENTRY* new_entry = &new_pt->entries[i];

            // Copiar flags básicas
            new_entry->valid = orig_entry->valid;
            new_entry->dirty = orig_entry->dirty;
            new_entry->referenced = orig_entry->referenced;

            // Tratar frames válidos
            if (orig_entry->valid) {
                // Alocar novo frame
                uintptr_t new_frame = (uintptr_t) nalloc_alloc(new_ctx, page_size);
                if (!new_frame) {
                    // Liberar recursos alocados até agora
                    for (uint32_t j = 0; j < i; j++) {
                        if (new_pt->entries[j].valid) {
                            nalloc_free(new_ctx, (void*)new_pt->entries[j].frame);
                        }
                    }
                    nalloc_free(new_ctx, new_pt->entries);
                    nalloc_free(new_ctx, new_pt);
                    return NULL;
                }

                // Copiar conteúdo do frame original
                memcpy((void*)new_frame, (void*)orig_entry->frame, page_size);
                new_entry->frame = new_frame;
            } else {
                new_entry->frame = INVALID_FRAME;
            }
        }
    }

    return new_pt;
}

