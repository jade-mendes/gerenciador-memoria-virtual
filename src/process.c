//
// Created by Nathan on 05/06/2025.
//

#include <stdint.h>

#include "process.h"

#include <stdio.h>
#include <string.h>

#include "Simulador.h"
#include "tabelas.h"


void destroy_process(Process* process, const NallocContext* nalloc_ctx) {
    if (!process) return;

    // Libera a tabela de páginas
    destroy_page_table(nalloc_ctx, process->page_table);

    // Libera o hashmap de variáveis
    hashmap_destroy(process->variables_adrr);

    if (process->instructions) {
        nalloc_free(nalloc_ctx, process->instructions);
        return;
    }

    // Libera a memória do processo
    nalloc_free(nalloc_ctx, process);
}


// Função auxiliar para inicializar um processo
void init_process(Process* proc, const uint32_t pid, const char* name, const uint32_t instruction_count, const Simulador* sim, const ProcessState state) {
    proc->pid = pid;
    strncpy(proc->name, name, sizeof(proc->name) - 1);
    proc->name[sizeof(proc->name) - 1] = '\0';
    proc->state = state;
    proc->instruction_index = 0;
    proc->instruction_count = instruction_count;
    proc->time_slice_remaining = sim->config.TIME_SLICE;
}

// Função auxiliar para alocar estruturas do processo
bool allocate_process_objects(Process* proc, NallocContext* alloc_ctx, const uint32_t instruction_count) {
    proc->page_table = create_page_table(alloc_ctx);
    proc->variables_adrr = hashmap_create(alloc_ctx, 10);
    proc->instructions = nalloc_alloc(alloc_ctx, instruction_count * sizeof(Instruction));

    return proc->page_table && proc->variables_adrr && proc->instructions;
}

// Função auxiliar para carregar texto na memória
bool load_process_text(const Simulador* sim, Process* proc, const char* texts, const int text_size) {
    const int num_pages = (text_size + sim->config.PAGE_SIZE - 1) / sim->config.PAGE_SIZE;

    for (int i = 0; i < num_pages; i++) {
        const uintptr_t virt_addr = i * sim->config.PAGE_SIZE;
        if (!allocate_page(sim, proc, virt_addr)) return false;

        for (int j = 0; j < sim->config.PAGE_SIZE; j++) {
            const int offset = i * sim->config.PAGE_SIZE + j;
            if (offset >= text_size) break;

            int status = 0;
            set_mem(sim, proc, virt_addr + j, texts[offset], false, &status);
            if (status != MEM_ACCESS_OK) return false;
        }
    }
    printf("Texto carregado no processo %s (PID: %u) com sucesso. ['%s']\n", proc->name, proc->pid, texts);
    fflush(stdout);
    return true;
}

// Função para criar processo suspenso (memória secundária)
Process* create_suspended_process(Simulador* sim, const uint32_t pid, const char* name, const Instruction* instructions, const uint32_t instruction_count, char* texts, const int text_size) {
    Process* proc = nalloc_alloc(&sim->secondary_memory_ctx, sizeof(Process));
    if (!proc) return NULL;

    init_process(proc, pid, name, instruction_count, sim, PROCESS_SUSPENDED);

    if (!allocate_process_objects(proc, &sim->secondary_memory_ctx, instruction_count)) {
        destroy_process(proc, &sim->secondary_memory_ctx);
        return NULL;
    }

    memcpy(proc->instructions, instructions, instruction_count * sizeof(Instruction));

    if (!load_process_text(sim, proc, texts, text_size)) {
        destroy_process(proc, &sim->secondary_memory_ctx);
        return NULL;
    }

    process_hashmap_put(sim->process_map_secondary, pid, proc);
    return proc;
}

// Função principal refatorada
Process* criar_processo(Simulador* sim, const uint32_t pid, const char* name, const Instruction* instructions, const uint32_t instruction_count, char* texts, const int text_size) {
    // Tenta criar na memória principal
    Process* proc = nalloc_alloc(&sim->main_memory_ctx, sizeof(Process));
    if (!proc) return create_suspended_process(sim, pid, name, instructions, instruction_count, texts, text_size);

    init_process(proc, pid, name, instruction_count, sim, PROCESS_READY);

    if (!allocate_process_objects(proc, &sim->main_memory_ctx, instruction_count)) {
        destroy_process(proc, &sim->main_memory_ctx);
        return create_suspended_process(sim, pid, name, instructions, instruction_count, texts, text_size);
    }

    memcpy(proc->instructions, instructions, instruction_count * sizeof(Instruction));

    if (!load_process_text(sim, proc, texts, text_size)) {
        destroy_process(proc, &sim->main_memory_ctx);
        return create_suspended_process(sim, pid, name, instructions, instruction_count, texts, text_size);
    }

    process_hashmap_put(sim->process_map_main, pid, proc);
    process_queue_enqueue(sim->process_queue, proc);
    return proc;
}

// Função para terminar um processo
void terminar_processo(Simulador* sim, Process* process) {
    if (!process) return;
    if (sim->current_process == process) {
        sim->current_process = NULL; // Libera a CPU se o processo atual for o que está sendo terminado
    }

    destroy_page_table(&sim->main_memory_ctx, process->page_table);
    hashmap_destroy(process->variables_adrr); // Função hipotética
    process_queue_remove(sim->process_queue, process); // Remove da fila de prontos

    if (process->state == PROCESS_SUSPENDED) {
        // Se o processo estava suspenso, remove da memória secundária
        process_hashmap_remove(sim->process_map_secondary, process->pid);
        destroy_process(process, &sim->secondary_memory_ctx);
    } else {
        // Se o processo estava na memória principal, remove do mapa de processos principal
        process_hashmap_remove(sim->process_map_main, process->pid);
        destroy_process(process, &sim->main_memory_ctx);
    }
}

// Função para bloquear o processo atual
void bloquear_processo_atual(Simulador* sim, const BlockReason reason, const uint32_t info) {
    // Atualiza estado
    sim->current_process->state = PROCESS_BLOCKED;

    sim->current_process->blocked_reason = reason;
    sim->current_process->blocked_reason_info = info;

    // Libera a CPU
    sim->current_process = NULL;
}

void desbloquear_processo(const Simulador* sim, Process* process) {
    if (!process) return;

    // Verifica se o processo está bloqueado
    if (process->state != PROCESS_BLOCKED) return;

    // Atualiza estado para pronto
    process->state = PROCESS_READY;

    // Adiciona à fila de prontos
    process_queue_enqueue(sim->process_queue, process);
}


bool try_swipe(Simulador* sim, Process* process) {
    if (!process) return false;

    if (process->state == PROCESS_SUSPENDED) {
        Process* new_process = nalloc_alloc(&sim->main_memory_ctx, sizeof(Process));
        if (!new_process) return false; // Falha na alocação

        new_process->pid = process->pid;
        strncpy(new_process->name, process->name, sizeof(new_process->name) - 1);
        new_process->name[sizeof(new_process->name) - 1] = '\0';
        new_process->state = PROCESS_READY;
        new_process->instruction_index = process->instruction_index;
        new_process->instruction_count = process->instruction_count;
        new_process->time_slice_remaining = sim->config.TIME_SLICE;

        new_process->variables_adrr = hashmap_clone(&sim->main_memory_ctx, process->variables_adrr);

        new_process->instructions = nalloc_alloc(&sim->main_memory_ctx, process->instruction_count * sizeof(Instruction));
        new_process->instruction_count = process->instruction_count;

        new_process->page_table = page_table_clone(&sim->main_memory_ctx, process->page_table, sim->config.PAGE_SIZE);

        if (!new_process->variables_adrr || !new_process->instructions || !new_process->page_table) {
            destroy_process(new_process, &sim->main_memory_ctx);
            return false; // Falha na alocação de alguma estrutura
        }
        // Copia as instruções
        memcpy(new_process->instructions, process->instructions, process->instruction_count * sizeof(Instruction));

        // Registra o novo processo na memória principal
        process_hashmap_put(sim->process_map_main, new_process->pid, new_process);

        // Remove o processo da memória secundária
        destroy_process(process, &sim->secondary_memory_ctx);
        if (process_hashmap_get(sim->process_map_secondary, process->pid, &process)) {
            process_hashmap_remove(sim->process_map_secondary, process->pid);
        }

        return true; // Sucesso na troca
    }

    // Se o processo não está suspenso, suspende
    Process* new_process = nalloc_alloc(&sim->secondary_memory_ctx, sizeof(Process));
    if (!new_process) return false; // Falha na alocação

    new_process->pid = process->pid;
    strncpy(new_process->name, process->name, sizeof(new_process->name) - 1);
    new_process->name[sizeof(new_process->name) - 1] = '\0';
    new_process->state = PROCESS_SUSPENDED;
    new_process->instruction_index = process->instruction_index;
    new_process->instruction_count = process->instruction_count;
    new_process->time_slice_remaining = sim->config.TIME_SLICE;

    new_process->variables_adrr = hashmap_clone(&sim->secondary_memory_ctx, process->variables_adrr);

    new_process->instructions = nalloc_alloc(&sim->secondary_memory_ctx, process->instruction_count * sizeof(Instruction));
    new_process->instruction_count = process->instruction_count;

    new_process->page_table = page_table_clone(&sim->secondary_memory_ctx, process->page_table, sim->config.PAGE_SIZE);

    if (!new_process->variables_adrr || !new_process->instructions || !new_process->page_table) {
        destroy_process(new_process, &sim->secondary_memory_ctx);
        return false; // Falha na alocação de alguma estrutura
    }
    // Copia as instruções
    memcpy(new_process->instructions, process->instructions, process->instruction_count * sizeof(Instruction));

    // Registra o novo processo na memória secundária
    process_hashmap_put(sim->process_map_secondary, new_process->pid, new_process);

    // Remove o processo da memória principal
    destroy_process(process, &sim->main_memory_ctx);
    if (process_hashmap_get(sim->process_map_main, process->pid, &process)) {
        process_hashmap_remove(sim->process_map_main, process->pid);
    }

    return true; // Sucesso na troca
}
