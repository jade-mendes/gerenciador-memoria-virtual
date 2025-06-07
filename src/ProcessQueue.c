//
// Created by Nathan Pinheiro on 05/06/2025.
//


#include "ProcessQueue.h"
#include "nalloc.h"
#include <stddef.h>

ProcessQueue* process_queue_create(NallocContext ctx) {
    ProcessQueue* queue = nalloc_alloc(&ctx, sizeof(ProcessQueue));
    if (!queue) return NULL;

    queue->nalloc_ctx = ctx;
    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;
    return queue;
}

bool process_queue_enqueue(ProcessQueue* queue, Process* process) {
    if (!queue || !process) return false;

    ProcessQueueNode* new_node = nalloc_alloc(&queue->nalloc_ctx, sizeof(ProcessQueueNode));
    if (!new_node) return false;

    new_node->process = process;
    new_node->next = NULL;

    if (queue->rear == NULL) {
        // Fila vazia
        queue->front = new_node;
        queue->rear = new_node;
    } else {
        // Adiciona ao final
        queue->rear->next = new_node;
        queue->rear = new_node;
    }

    queue->size++;
    return true;
}

Process* process_queue_dequeue(ProcessQueue* queue) {
    if (!queue || process_queue_is_empty(queue)) return NULL;

    ProcessQueueNode* temp = queue->front;
    Process* process = temp->process;

    // Atualiza o início da fila
    queue->front = queue->front->next;

    // Se a fila ficar vazia
    if (queue->front == NULL) {
        queue->rear = NULL;
    }

    // Libera o nó, mas mantém o processo
    nalloc_free(&queue->nalloc_ctx, temp);
    queue->size--;
    return process;
}

bool process_queue_remove(ProcessQueue* queue, Process* target) {
    if (queue == NULL || target == NULL || queue->size == 0) {
        return false;
    }

    ProcessQueueNode* current = queue->front;
    ProcessQueueNode* prev = NULL;
    bool found = false;

    // Percorre a fila procurando pelo processo alvo
    while (current != NULL) {
        if (current->process == target) {
            found = true;
            break;
        }
        prev = current;
        current = current->next;
    }

    if (!found) {
        return false;
    }

    // Atualiza os ponteiros da fila
    if (prev == NULL) {
        // Remoção do primeiro elemento
        queue->front = current->next;
    } else {
        prev->next = current->next;
    }

    // Atualiza o rear se for o último elemento
    if (current == queue->rear) {
        queue->rear = prev;
    }

    // Libera o nó e atualiza o tamanho
    nalloc_free(&queue->nalloc_ctx, current);
    queue->size--;

    return true;
}

bool process_queue_is_empty(const ProcessQueue* queue) {
    return queue ? (queue->front == NULL) : true;
}

size_t process_queue_size(const ProcessQueue* queue) {
    return queue ? queue->size : 0;
}

void process_queue_destroy(ProcessQueue* queue) {
    if (!queue) return;

    // Desenfileira todos os elementos restantes
    while (!process_queue_is_empty(queue)) {
        process_queue_dequeue(queue);
    }

    // Libera a estrutura principal
    nalloc_free(&queue->nalloc_ctx, queue);
}