//
// Created by Nathan on 05/06/2025.
//

#include <stdint.h>

#include "process.h"

#include <string.h>

#include "Simulador.h"
#include "tabelas.h"


void destroy_process(Process* process, const NallocContext* nalloc_ctx) {
    if (!process) return;

    // Libera a tabela de páginas
    destroy_page_table(nalloc_ctx, process->page_table);

    // Libera o hashmap de variáveis
    hashmap_destroy(process->variables_adrr);

    // Libera a memória do processo
    nalloc_free(nalloc_ctx, process);
}


// Função para criar um novo processo
Process* criar_processo(Simulador* sim, uint32_t pid, const char* name, Instruction* instructions, uint32_t instruction_count, char* texts, int text_size) {
    // Aloca memória para o processo na memória principal
    Process* new_process = nalloc_alloc(&sim->main_memory_ctx, sizeof(Process));
    if (!new_process) return NULL;

    // Inicializa os campos do processo
    new_process->pid = pid;
    strncpy(new_process->name, name, sizeof(new_process->name) - 1);
    new_process->name[sizeof(new_process->name) - 1] = '\0';
    new_process->state = PROCESS_READY;
    new_process->instruction_index = 0;
    new_process->instruction_count = instruction_count;
    new_process->time_slice_remaining = sim->config.TIME_SLICE;

    // Inicializa estruturas de dados do processo
    new_process->page_table = create_page_table(&sim->main_memory_ctx);
    new_process->variables_adrr = hashmap_create(&sim->main_memory_ctx, 10);

    // Alloca as instruções
    new_process->instructions = nalloc_alloc(&sim->main_memory_ctx, instruction_count * sizeof(Instruction));
    if (!new_process->instructions) {
        nalloc_free(&sim->main_memory_ctx, new_process);
        return NULL;
    }
    memcpy(new_process->instructions, instructions, instruction_count * sizeof(Instruction));

    // Adiciona ao mapa de processos principal
    process_hashmap_put(sim->process_map_main, pid, new_process);

    // Adiciona à fila de processos prontos
    process_queue_enqueue(sim->process_queue, new_process);

    // texts
    const int num_pages = (text_size + sim->config.PAGE_SIZE - 1) / sim->config.PAGE_SIZE;
    for (int i = 0; i < num_pages; i++) {
        uintptr_t virt_addr = i * sim->config.PAGE_SIZE;
        if (!allocate_page(sim, new_process, virt_addr)) {
            destroy_process(new_process, &sim->main_memory_ctx);
            return NULL;
        }
        for (int j = 0; j < sim->config.PAGE_SIZE && (i * sim->config.PAGE_SIZE + j) < text_size; j++) {
            int status = 0;
            set_mem(sim, new_process, virt_addr + j, texts[i * sim->config.PAGE_SIZE + j], &status);
            if (status != MEM_ACCESS_OK) {
                destroy_process(new_process, &sim->main_memory_ctx);
                return NULL;
            }
        }
    }

    return new_process;
}

// Função para terminar um processo
void terminar_processo(Simulador* sim, Process* process) {
    destroy_page_table(&sim->main_memory_ctx, process->page_table);
    hashmap_destroy(process->variables_adrr); // Função hipotética
    process_queue_remove(sim->process_queue, process); // Remove da fila de prontos

    if (process->state == PROCESS_SUSPENDED) {
        // Se o processo estava suspenso, remove da memória secundária
        process_hashmap_remove(sim->process_map_secondary, process->pid);
        nalloc_free(&sim->secondary_memory_ctx, process->instructions);
    } else {
        // Se o processo estava na memória principal, remove do mapa de processos principal
        process_hashmap_remove(sim->process_map_main, process->pid);
        nalloc_free(&sim->main_memory_ctx, process->instructions);
    }

    // Libera a memória do processo
    nalloc_free(&sim->main_memory_ctx, process);
}

// Função para desuspender um processo
void desuspender_processo(const Simulador* sim, uint32_t pid) {
    Process* process = NULL;

    // Só pode desuspender processos na memória secundária
    if (!process_hashmap_get(sim->process_map_secondary, pid, &process)) return;

    // Aloca espaço na memória principal
    Process* main_copy = nalloc_alloc(&sim->main_memory_ctx, sizeof(Process));
    if (!main_copy) return;

    // Copia os dados do processo
    memcpy(main_copy, process, sizeof(Process));
    main_copy->state = PROCESS_READY;

    // Adiciona na memória principal
    process_hashmap_put(sim->process_map_main, pid, main_copy);

    // Adiciona na fila de prontos
    process_queue_enqueue(sim->process_queue, main_copy);

    // Libera memória secundária
    process_hashmap_remove(sim->process_map_secondary, pid);
    nalloc_free(&sim->secondary_memory_ctx, process);
}

// Função para bloquear um processo
void bloquear_processo(Simulador* sim, uint32_t pid, BlockReason reason, uint32_t info) {
    // Só pode bloquear o processo atualmente em execução
    if (!sim->current_process || sim->current_process->pid != pid) return;

    // Atualiza estado
    sim->current_process->state = PROCESS_BLOCKED;

    sim->current_process->blocked_reason = reason;
    sim->current_process->blocked_reason_info = info;

    // Libera a CPU
    sim->current_process = NULL;
}

