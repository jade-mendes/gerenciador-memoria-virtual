//
// Created by Nathan Pinheiro on 03/06/2025.
//
#pragma once
#include <stddef.h>
#include "n.h"

#ifndef N_INTERPRETER_H
#define N_INTERPRETER_H

typedef struct Simulador Simulador;
typedef struct Process Process;


#define MAX_MSG_SIZE 256
extern char process_input[MAX_MSG_SIZE];
extern char process_output[MAX_MSG_SIZE];
extern char process_error[MAX_MSG_SIZE];



void execute(const size_t index, const size_t pid, Instruction *instructions);


#endif //N_INTERPRETER_H
