//
// Created by Nathan Pinheiro on 05/06/2025.
//
#pragma once

#include <stdint.h>
#include "nalloc.h"
#include "ProcessHashMap.h"
#include "ProcessQueue.h"
#ifndef SIMULADOR_H

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

typedef struct {
    SimulationConfig config;
    ProcessHashMap* process_map_main;
    ProcessHashMap* process_map_secondary;
    ProcessQueue* process_queue;

    Process* current_process;

    NallocContext *main_memory_ctx;
    NallocContext *secondary_memory_ctx;

    TLB* tlb;
} Simulador;

#define SIMULADOR_H

#endif //SIMULADOR_H
