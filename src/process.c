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


// Função para criar um novo processo
Process* criar_processo(Simulador* sim, uint32_t pid, const char* name, Instruction* instructions, uint32_t instruction_count, char* texts, int text_size) {
    // Primeiro tenta criar na memória principal
    Process* new_process = nalloc_alloc(&sim->main_memory_ctx, sizeof(Process));
    if (!new_process) goto criar_secundario; // Falha, tenta secundária

    // Inicialização básica
    new_process->pid = pid;
    strncpy(new_process->name, name, sizeof(new_process->name) - 1);
    new_process->name[sizeof(new_process->name) - 1] = '\0';
    new_process->state = PROCESS_READY;
    new_process->instruction_index = 0;
    new_process->instruction_count = instruction_count;
    new_process->time_slice_remaining = sim->config.TIME_SLICE;

    // Aloca estruturas na memória principal
    new_process->page_table = create_page_table(&sim->main_memory_ctx);
    new_process->variables_adrr = hashmap_create(&sim->main_memory_ctx, 10);
    new_process->instructions = nalloc_alloc(&sim->main_memory_ctx, instruction_count * sizeof(Instruction));

    // Verifica falhas na alocação
    if (!new_process->page_table || !new_process->variables_adrr || !new_process->instructions) {
        destroy_process(new_process, &sim->main_memory_ctx);
        goto criar_secundario;
    }

    // Copia instruções
    memcpy(new_process->instructions, instructions, instruction_count * sizeof(Instruction));

    // Aloca e carrega páginas de texto
    int num_pages = (text_size + sim->config.PAGE_SIZE - 1) / sim->config.PAGE_SIZE;
    for (int i = 0; i < num_pages; i++) {
        uintptr_t virt_addr = i * sim->config.PAGE_SIZE;
        if (!allocate_page(sim, new_process, virt_addr)) {
            destroy_process(new_process, &sim->main_memory_ctx);
            goto criar_secundario;
        }
        for (int j = 0; j < sim->config.PAGE_SIZE && (i * sim->config.PAGE_SIZE + j) < text_size; j++) {
            int status = 0;
            set_mem(sim, new_process, virt_addr + j, texts[i * sim->config.PAGE_SIZE + j], &status);
            if (status != MEM_ACCESS_OK) {
                destroy_process(new_process, &sim->main_memory_ctx);
                goto criar_secundario;
            }
        }
    }

    // Sucesso na memória principal
    process_hashmap_put(sim->process_map_main, pid, new_process);
    process_queue_enqueue(sim->process_queue, new_process);
    return new_process;

    criar_secundario:
    // Tenta criar como suspenso na memória secundária
    new_process = nalloc_alloc(&sim->secondary_memory_ctx, sizeof(Process));
    if (!new_process) return NULL; // Falha total

    // Inicialização como suspenso
    new_process->pid = pid;
    strncpy(new_process->name, name, sizeof(new_process->name) - 1);
    new_process->name[sizeof(new_process->name) - 1] = '\0';
    new_process->state = PROCESS_SUSPENDED;
    new_process->instruction_index = 0;
    new_process->instruction_count = instruction_count;
    new_process->time_slice_remaining = sim->config.TIME_SLICE;

    // Aloca estruturas na memória secundária
    new_process->page_table = create_page_table(&sim->secondary_memory_ctx);
    new_process->variables_adrr = hashmap_create(&sim->secondary_memory_ctx, 10);
    new_process->instructions = nalloc_alloc(&sim->secondary_memory_ctx, instruction_count * sizeof(Instruction));

    // Verifica falhas
    if (!new_process->page_table || !new_process->variables_adrr || !new_process->instructions) {
        destroy_process(new_process, &sim->secondary_memory_ctx);
        return NULL;
    }

    // Copia instruções
    memcpy(new_process->instructions, instructions, instruction_count * sizeof(Instruction));

    // Cria entradas na tabela de páginas (sem alocar frames físicos)
    num_pages = (text_size + sim->config.PAGE_SIZE - 1) / sim->config.PAGE_SIZE;
    for (int i = 0; i < num_pages; i++) {
        uint32_t page_index = i;
        if (page_index >= new_process->page_table->num_entries) {
            // Expande tabela se necessário
            uint32_t new_size = page_index + 1;
            PAGE_TABLE_ENTRY* new_entries = nalloc_realloc(&sim->secondary_memory_ctx,
                new_process->page_table->entries,
                new_size * sizeof(PAGE_TABLE_ENTRY));

            if (!new_entries) {
                destroy_process(new_process, &sim->secondary_memory_ctx);
                return NULL;
            }

            // Inicializa novas entradas
            for (uint32_t j = new_process->page_table->num_entries; j < new_size; j++) {
                new_entries[j].valid = false;
                new_entries[j].dirty = false;
                new_entries[j].referenced = false;
                new_entries[j].frame = INVALID_FRAME;
            }

            new_process->page_table->entries = new_entries;
            new_process->page_table->num_entries = new_size;
        }
    }

    // Registra na memória secundária
    process_hashmap_put(sim->process_map_secondary, pid, new_process);
    return new_process;
}

// Função para terminar um processo
void terminar_processo(Simulador* sim, Process* process) {
    if (!process) return;

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
