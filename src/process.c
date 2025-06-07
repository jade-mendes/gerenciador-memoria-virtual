//
// Created by Nathan on 05/06/2025.
//

#include <stdint.h>

#include "process.h"

#include <string.h>

#include "Simulador.h"



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
    new_process->instructions = instructions;
    new_process->instruction_count = instruction_count;
    new_process->time_slice_remaining = sim->config.TIME_SLICE;

    // Inicializa estruturas de dados do processo
    new_process->page_table = create_page_table(&sim->main_memory_ctx);
    new_process->variables_adrr = hashmap_create(&sim->main_memory_ctx, 10);

    // Adiciona ao mapa de processos principal
    process_hashmap_put(sim->process_map_main, pid, new_process);

    // Adiciona à fila de processos prontos
    process_queue_enqueue(sim->process_queue, new_process);

    // texts
    for (int i = 0; i < text_size / sim->config.PAGE_SIZE; i++) {
        uintptr_t virt_addr = i * sim->config.PAGE_SIZE;
        if (!allocate_page(sim, new_process, virt_addr)) {
            // Se falhar na alocação de página, libera o processo e retorna NULL
            destroy_process(new_process, &sim->main_memory_ctx);
            return NULL;
        }
        // Copia o texto para a memória virtual do processo
        for (int j = 0; j < sim->config.PAGE_SIZE && (i * sim->config.PAGE_SIZE + j) < text_size; j++) {
            int status = 0;
            set_mem(sim, new_process, virt_addr + j, texts[i * sim->config.PAGE_SIZE + j], &status);
            if (status != MEM_ACCESS_OK) {
                // Se falhar ao definir a memória, libera o processo e retorna NULL
                destroy_process(new_process, &sim->main_memory_ctx);
                return NULL;
            }
        }

    }

    return new_process;
}

void destroy_process(Process* process, const NallocContext* nalloc_ctx) {
    if (!process) return;

    // Libera a tabela de páginas
    destroy_page_table(nalloc_ctx, process->page_table);

    // Libera o hashmap de variáveis
    hashmap_destroy(process->variables_adrr);

    // Libera a memória do processo
    nalloc_free(nalloc_ctx, process);
}

// Função para terminar um processo
void terminar_processo(Simulador* sim, uint32_t pid) {
    Process* process = NULL;

    // Procura o processo na memória principal
    if (process_hashmap_get(sim->process_map_main, pid, &process)) {
        // Remove das estruturas ativas
        process_hashmap_remove(sim->process_map_main, pid);
        if (sim->current_process == process) {
            sim->current_process = NULL;
        }

        // Libera recursos
        destroy_page_table(&sim->main_memory_ctx, process->page_table);
        hashmap_destroy(process->variables_adrr); // Função hipotética
        nalloc_free(&sim->main_memory_ctx, process);
    }
    // Procura na memória secundária
    else if (process_hashmap_get(sim->process_map_secondary, pid, &process)) {
        process_hashmap_remove(sim->process_map_secondary, pid);

        // Libera recursos
        destroy_page_table(&sim->secondary_memory_ctx, process->page_table);
        hashmap_destroy(process->variables_adrr);
        nalloc_free(&sim->secondary_memory_ctx, process);
    }
}

// Função para suspender um processo
void suspender_processo(Simulador* sim, uint32_t pid) {
    Process* process = NULL;

    // Só pode suspender processos na memória principal
    if (!process_hashmap_get(sim->process_map_main, pid, &process)) return;

    // Remove da fila de processos se estiver nela
    if (process->state == PROCESS_READY) {
        process_queue_remove(sim->process_queue, process);
    }

    // Atualiza estado
    process->state = PROCESS_SUSPENDED;

    // Remove da memória principal
    process_hashmap_remove(sim->process_map_main, pid);

    // Aloca espaço na memória secundária
    Process* secondary_copy = nalloc_alloc(&sim->secondary_memory_ctx, sizeof(Process));
    if (!secondary_copy) return;

    // Copia os dados do processo
    memcpy(secondary_copy, process, sizeof(Process));

    // Adiciona na memória secundária
    process_hashmap_put(sim->process_map_secondary, pid, secondary_copy);

    // Libera memória principal
    nalloc_free(&sim->main_memory_ctx, process);

    // Atualiza ponteiro se for o processo atual
    if (sim->current_process && sim->current_process->pid == pid) {
        sim->current_process = NULL;
    }
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
void bloquear_processo(Simulador* sim, uint32_t pid) {
    // Só pode bloquear o processo atualmente em execução
    if (!sim->current_process || sim->current_process->pid != pid) return;

    // Atualiza estado
    sim->current_process->state = PROCESS_BLOCKED;

    // Não é necessário remover da fila pois processos bloqueados
    // não estão na fila de prontos por definição

    // Libera a CPU
    sim->current_process = NULL;
}

// Função para desbloquear um processo
void desbloquear_processo(const Simulador* sim, uint32_t pid) {
    Process* process = NULL;

    // Procura na memória principal (processos bloqueados permanecem na memória)
    if (process_hashmap_get(sim->process_map_main, pid, &process)) {
        // Verifica se está bloqueado
        if (process->state == PROCESS_BLOCKED) {
            process->state = PROCESS_READY;
            process_queue_enqueue(sim->process_queue, process);
        }
    }
}

