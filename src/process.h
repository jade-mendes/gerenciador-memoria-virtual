//
// Created by Nathan on 03/06/2025.
//
#pragma once

#include <stdint.h>
#include "HashMap.h"

#ifndef PROCESS_H
typedef struct {
    uint32_t pid;          // Identificador do processo
    char name[32];        // Nome do processo
    // PAGE_TABLE *page_table; // Tabela de páginas do processo

    HashMap* variables_adrr; // Endereço das variáveis do processo
} Process;

#define PROCESS_H

#endif //PROCESS_H
