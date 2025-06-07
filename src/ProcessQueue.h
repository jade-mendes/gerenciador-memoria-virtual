//
// Created by Nathan Pinheiro on 05/06/2025.
//
#pragma once


#ifndef PROCESSQUEUE_H
#define PROCESSQUEUE_H

#include "nalloc.h"

typedef struct Process Process;


typedef struct ProcessQueueNode {
    Process* process;
    struct ProcessQueueNode* next;
} ProcessQueueNode;

typedef struct {
    NallocContext nalloc_ctx;  // Contexto de alocação
    ProcessQueueNode* front;     // Primeiro elemento da fila
    ProcessQueueNode* rear;      // Último elemento da fila
    size_t size;                 // Tamanho atual da fila
} ProcessQueue;

// Cria uma nova fila vazia
ProcessQueue* process_queue_create(NallocContext ctx);

// Adiciona um processo ao final da fila
bool process_queue_enqueue(ProcessQueue* queue, Process* process);

// Remove e retorna o processo do início da fila
Process* process_queue_dequeue(ProcessQueue* queue);

// Remove um processo específico da fila retornando true se encontrado e removido
bool process_queue_remove(ProcessQueue* queue, Process* target);

// Verifica se a fila está vazia
bool process_queue_is_empty(const ProcessQueue* queue);

// Retorna o tamanho atual da fila
size_t process_queue_size(const ProcessQueue* queue);

// Libera todos os recursos da fila (não destrói os processos)
void process_queue_destroy(ProcessQueue* queue);



// Macro para iteração simples (não segura para remoção durante a iteração)
#define PROCESS_QUEUE_FOREACH(queue, node_var) \
for (ProcessQueueNode* node_var = (queue)->front; \
node_var != NULL; \
node_var = node_var->next)

// Macro para iteração segura (permite remoção durante a iteração)
#define PROCESS_QUEUE_FOREACH_SAFE(queue, node_var, next_var) \
for (ProcessQueueNode *node_var = (queue)->front, \
*next_var = (node_var ? node_var->next : NULL); \
node_var != NULL; \
node_var = next_var, \
next_var = (node_var ? node_var->next : NULL))


#endif //PROCESSQUEUE_H
