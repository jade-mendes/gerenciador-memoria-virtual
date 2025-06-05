//
// Created by Nathan on 03/06/2025.
//
#pragma once

#include <stdint.h>
#include "HashMap.h"
#include "n.h"

#ifndef PROCESS_H

typedef enum {
    PROCESS_RUNNING,
    PROCESS_READY,
    PROCESS_SUSPENDED,
    PROCESS_BLOCKED,
} ProcessState;


typedef struct {
    uint32_t pid;
    char name[16];
    PAGE_TABLE *page_table;

    ProcessState state;

    HashMap* variables_adrr;
    uint32_t instruction_index;
    Instruction* instructions;
    uint32_t instruction_count;

    uint32_t time_slice_remaining;
} Process;

#define PROCESS_H

#endif //PROCESS_H
