//
// Created by Nathan Pinheiro on 05/06/2025.
//
#pragma once

#ifndef SIMULADOR_H
#define SIMULADOR_H

#include <stdint.h>
#include "nalloc.h"
#include "ProcessHashMap.h"
#include "ProcessQueue.h"



// Definições de status de acesso à memória
#define MEM_ACCESS_OK 0
#define MEM_ACCESS_PAGE_NOT_ALLOCATED 1
#define INVALID_FRAME 0xFFFFFFFF

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

typedef struct {
    TLB_ENTRY* entries;
    uint32_t size;
    uint64_t counter;
} TLB;

typedef enum {
    SUB_POLICY_LRU,
    SUB_POLICY_CLOCK
} SubPolicyType;

typedef struct {
    uint32_t PAGE_SIZE;
    uint32_t MP_SIZE;
    uint32_t MS_SIZE;
    uint32_t TLB_SIZE;
    uint32_t TIME_SLICE;
    SubPolicyType BITS_LOGICAL_ADDRESS;
    uint32_t SUB_POLICY_TYPE;
    char FILE_NAME[100];
} SimulationConfig;


typedef struct Simulador {
    SimulationConfig config;
    ProcessHashMap* process_map_main;
    ProcessHashMap* process_map_secondary;
    ProcessQueue* process_queue;

    Process* current_process;

    NallocContext main_memory_ctx;
    NallocContext secondary_memory_ctx;

    TLB* tlb;
} Simulador;

// Funções de gerenciamento da TLB
bool tlb_lookup(TLB* tlb, const uint32_t page, uintptr_t* frame);
void tlb_update(TLB* tlb, const uint32_t page, const uint32_t frame);
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
TLB* create_tlb(NallocContext* ctx, uint32_t size);
void destroy_tlb(const NallocContext* ctx, TLB* tlb);

// Função para destruir páginas de um processo
void destroy_process_pages(Simulador* s, Process* p);



#endif //SIMULADOR_H
