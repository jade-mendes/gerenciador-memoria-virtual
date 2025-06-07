//
// Created by Nathan Pinheiro on 03/06/2025.
//
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "n.h"

#ifndef N_INTERPRETER_H
#define N_INTERPRETER_H

typedef struct Simulador Simulador;
typedef struct Process Process;



void execute(const size_t index, const size_t pid, Instruction *instructions);


#endif //N_INTERPRETER_H
