//
// Created by natha on 08/06/2025.
//

#ifndef TABELAS_H
#define TABELAS_H

#include <stdbool.h>
#include <stdint.h>
#include "nalloc.h"

typedef struct Simulador Simulador;
typedef struct Process Process;

// Definições de status de acesso à memória
#define MEM_ACCESS_OK 0
#define MEM_ACCESS_PAGE_NOT_ALLOCATED 1
#define MEM_ACCESS_INVALID_ADDRESS 2
#define VAR_NOT_FOUND 3
#define INVALID_FRAME 0xFFFFFFFF

#define ADDR_STATUS(status) (status == MEM_ACCESS_OK ? "OK" : \
                             status == MEM_ACCESS_PAGE_NOT_ALLOCATED ? "PAGE NOT ALLOCATED" : \
                             status == MEM_ACCESS_INVALID_ADDRESS ? "INVALID ADDRESS" : \
                             status == VAR_NOT_FOUND ? "VAR NOT FOUND" : "UNKNOWN ERROR")

// Estruturas da tabela de páginas
typedef struct {
    uintptr_t frame;
    bool valid;
    bool dirty;
    bool referenced;
} PAGE_TABLE_ENTRY;

typedef struct PAGE_TABLE {
    PAGE_TABLE_ENTRY* entries;
    uint32_t num_entries;
} PAGE_TABLE;

// Estruturas da TLB
typedef struct {
    uint32_t page;
    uintptr_t frame;
    bool valid;
    uint64_t last_used;
} TLB_ENTRY;

typedef struct TLB {
    TLB_ENTRY* entries;
    uint32_t size;
    uint64_t counter;
} TLB ;


// Funções de gerenciamento da TLB
bool tlb_lookup(TLB* tlb, const uint32_t page, uintptr_t* frame);
void tlb_update(TLB* tlb, const uint32_t page, const uintptr_t frame);
void tlb_invalidate_entry(const TLB* tlb, const uint32_t page);
void reset_tlb_validity(const TLB* tlb);

// Funções de acesso à memória
uint8_t get_mem(const Simulador* s, Process* p, const uint32_t virt_addr, int* out_status);
void set_mem(const Simulador* s, Process* p, const uint32_t virt_addr, const uint8_t value, int* out_status);

// Funções de gerenciamento de páginas
bool allocate_page(const Simulador* s, Process* p, uintptr_t virt_addr);
void deallocate_page(Simulador* s, Process* p, uint32_t virt_addr);

// Funções de criação/destruição
PAGE_TABLE* create_page_table(const NallocContext* ctx);
void destroy_page_table(const NallocContext* ctx, PAGE_TABLE* pt);
PAGE_TABLE* page_table_clone(const NallocContext* new_ctx, const PAGE_TABLE* original_pt, const uint32_t page_size);

TLB* create_tlb(NallocContext* ctx, uint32_t size);
void destroy_tlb(const NallocContext* ctx, TLB* tlb);


// Função para destruir páginas de um processo
void destroy_process_pages(Simulador* s, Process* p);

#endif //TABELAS_H
