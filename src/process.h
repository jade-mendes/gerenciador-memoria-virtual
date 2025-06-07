//
// Created by Nathan on 03/06/2025.
//
#pragma once




#ifndef PROCESS_H
#include <stdint.h>
#include "HashMap.h"
#include "n.h"

typedef struct Simulador Simulador;
typedef struct PAGE_TABLE PAGE_TABLE;

typedef enum {
    PROCESS_RUNNING,
    PROCESS_READY,
    PROCESS_SUSPENDED,
    PROCESS_BLOCKED,
} ProcessState;


typedef struct Process {
    uint32_t pid;
    char name[16];
    PAGE_TABLE* page_table;

    ProcessState state;

    HashMap* variables_adrr;
    uint32_t instruction_index;
    Instruction* instructions;
    uint32_t instruction_count;

    uint32_t time_slice_remaining;
} Process;


// Funções de gerenciamento de processos
Process* criar_processo(Simulador* sim, uint32_t pid, const char* name, Instruction* instructions, uint32_t instruction_count, char* texts, int text_size);
void destroy_process(Process* process, const NallocContext* nalloc_ctx);
void terminar_processo(Simulador* sim, uint32_t pid);
void suspender_processo(Simulador* sim, uint32_t pid);
void desuspender_processo(const Simulador* sim, uint32_t pid);
void bloquear_processo(Simulador* sim, uint32_t pid);
void desbloquear_processo(const Simulador* sim, uint32_t pid);

#define PROCESS_H

#endif //PROCESS_H
