//
// Created by Nathan Pinheiro on 05/06/2025.
//
#pragma once


#ifndef PROCESSQUEUE_H
#include "process.h"
#include "nalloc.h"

struct Process;

typedef struct ProcessQueueNode {
    struct Process* process;
    struct ProcessQueueNode* next;
} ProcessQueueNode;

typedef struct {
    NallocContext* nalloc_ctx;  // Contexto de alocação
    ProcessQueueNode* front;     // Primeiro elemento da fila
    ProcessQueueNode* rear;      // Último elemento da fila
    size_t size;                 // Tamanho atual da fila
} ProcessQueue;

// Cria uma nova fila vazia
ProcessQueue* process_queue_create(NallocContext* ctx);

// Adiciona um processo ao final da fila
bool process_queue_enqueue(ProcessQueue* queue, struct Process* process);

// Remove e retorna o processo do início da fila
struct Process* process_queue_dequeue(ProcessQueue* queue);

// Verifica se a fila está vazia
bool process_queue_is_empty(const ProcessQueue* queue);

// Retorna o tamanho atual da fila
size_t process_queue_size(const ProcessQueue* queue);

// Libera todos os recursos da fila (não destrói os processos)
void process_queue_destroy(ProcessQueue* queue);


#define PROCESSQUEUE_H

#endif //PROCESSQUEUE_H
