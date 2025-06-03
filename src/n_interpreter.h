//
// Created by Nathan Pinheiro on 03/06/2025.
//
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "n.h"

#ifndef N_INTERPRETER_H
void execute(const size_t index, const size_t pid, Instruction *instructions);
#define N_INTERPRETER_H

#endif //N_INTERPRETER_H
