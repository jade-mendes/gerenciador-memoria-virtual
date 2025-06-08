//
// Created by nathan on 08/06/2025.
//

#include "tabelas.h"
#include "process.h"
#include "Simulador.h"

// Funções de gerenciamento da TLB
bool tlb_lookup(TLB* tlb, const uint32_t page, uintptr_t* frame) {
    for (uint32_t i = 0; i < tlb->size; i++) {
        if (tlb->entries[i].valid && tlb->entries[i].page == page) {
            *frame = tlb->entries[i].frame;
            tlb->entries[i].last_used = tlb->counter++;
            return true;
        }
    }
    return false;
}

void tlb_update(TLB* tlb, const uint32_t page, const uintptr_t frame) {
    uint32_t lru_index = 0;
    uint64_t min_used = UINT64_MAX;

    for (uint32_t i = 0; i < tlb->size; i++) {
        if (!tlb->entries[i].valid) {
            lru_index = i;
            break;
        }
        if (tlb->entries[i].last_used < min_used) {
            min_used = tlb->entries[i].last_used;
            lru_index = i;
        }
    }

    tlb->entries[lru_index].page = page;
    tlb->entries[lru_index].frame = frame;
    tlb->entries[lru_index].valid = true;
    tlb->entries[lru_index].last_used = tlb->counter++;
}

void tlb_invalidate_entry(const TLB* tlb, const uint32_t page) {
    for (uint32_t i = 0; i < tlb->size; i++) {
        if (tlb->entries[i].valid && tlb->entries[i].page == page) {
            tlb->entries[i].valid = false;
            break;
        }
    }
}

void reset_tlb_validity(const TLB* tlb) {
    for (uint32_t i = 0; i < tlb->size; i++) {
        tlb->entries[i].valid = false;
    }
}

// Funções de acesso à memória
uint8_t get_mem(const Simulador* s, Process* p, const uint32_t virt_addr, int* out_status) {
    // Verificar se o endereço excede o espaço lógico
    const uint32_t max_virt_addr = (1U << s->config.BITS_LOGICAL_ADDRESS) - 1;
    if (virt_addr > max_virt_addr) {
        *out_status = MEM_ACCESS_INVALID_ADDRESS;
        return 0;
    }

    uint32_t page_num = virt_addr / s->config.PAGE_SIZE;
    const uint32_t offset = virt_addr % s->config.PAGE_SIZE;

    uintptr_t frame_addr;
    bool tlb_hit = tlb_lookup(s->tlb, page_num, &frame_addr);

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
        tlb_update(s->tlb, page_num, frame_addr);
    }

    entry->referenced = true;

    const uint8_t* physical_addr = (uint8_t*)(frame_addr + offset);
    *out_status = MEM_ACCESS_OK;
    return *physical_addr;
}

void set_mem(const Simulador* s, Process* p, const uint32_t virt_addr, const uint8_t value, int* out_status) {
    // Verificar se o endereço excede o espaço lógico
    const uint32_t max_virt_addr = (1U << s->config.BITS_LOGICAL_ADDRESS) - 1;
    if (virt_addr > max_virt_addr) {
        *out_status = MEM_ACCESS_INVALID_ADDRESS;
        return;
    }
    uint32_t page_num = virt_addr / s->config.PAGE_SIZE;
    uint32_t offset = virt_addr % s->config.PAGE_SIZE;

    uintptr_t frame_addr;
    bool tlb_hit = tlb_lookup(s->tlb, page_num, &frame_addr);

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
        tlb_update(s->tlb, page_num, frame_addr);
    }

    entry->referenced = true;
    entry->dirty = true;

    uint8_t* physical_addr = (uint8_t*)(frame_addr + offset);
    *physical_addr = value;
    *out_status = MEM_ACCESS_OK;
}

// Funções de gerenciamento de páginas ==============

// Aloca uma página virtual para o processo
// Pode ser chamada apenas por processos na memória principal
bool allocate_page(const Simulador* s, Process* p, uintptr_t virt_addr) {
    uint32_t page_num = virt_addr / s->config.PAGE_SIZE;

    if (page_num < p->page_table->num_entries && p->page_table->entries[page_num].valid) {
        return true; // Página já alocada
    }

    uintptr_t frame_addr = (uintptr_t) nalloc_alloc(&s->main_memory_ctx, s->config.PAGE_SIZE);
    if (!frame_addr) {
        return false; // Falha na alocação de memória
    }

    if (page_num >= p->page_table->num_entries) {
        uint32_t new_num_entries = page_num + 1;
        PAGE_TABLE_ENTRY* new_entries = nalloc_realloc(
            &s->main_memory_ctx,
            p->page_table->entries,
            new_num_entries * sizeof(PAGE_TABLE_ENTRY)
        );

        if (!new_entries) {
            nalloc_free(&s->main_memory_ctx, (void*) frame_addr);
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
    uint32_t page_num = virt_addr / s->config.PAGE_SIZE;

    if (page_num >= p->page_table->num_entries || !p->page_table->entries[page_num].valid) {
        return;
    }

    void* frame_addr = (void*) p->page_table->entries[page_num].frame;

    nalloc_free(&s->main_memory_ctx, frame_addr);
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
    if (pt->entries) {
        nalloc_free(ctx, pt->entries);
    }
    nalloc_free(ctx, pt);
}

TLB* create_tlb(NallocContext* ctx, const uint32_t size) {
    TLB* tlb = nalloc_alloc(ctx, sizeof(TLB));
    if (!tlb) return NULL;

    tlb->size = size;
    tlb->counter = 0;
    tlb->entries = (TLB_ENTRY*)nalloc_alloc(ctx, size * sizeof(TLB_ENTRY));

    if (!tlb->entries) {
        nalloc_free(ctx, tlb);
        return NULL;
    }

    for (uint32_t i = 0; i < size; i++) {
        tlb->entries[i].valid = false;
    }

    return tlb;
}

void destroy_tlb(const NallocContext* ctx, TLB* tlb) {
    if (tlb->entries) {
        nalloc_free(ctx, tlb->entries);
    }
    nalloc_free(ctx, tlb);
}

// Função para destruir páginas de um processo
void destroy_process_pages(Simulador* s, Process* p) {
    for (uint32_t i = 0; i < p->page_table->num_entries; i++) {
        if (p->page_table->entries[i].valid) {
            deallocate_page(s, p, i * s->config.PAGE_SIZE);
        }
    }
}

