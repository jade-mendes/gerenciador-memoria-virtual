//
// Created by nathan on 05/06/2025.
//


#include "Simulador.h"
#include "process.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "n_interpreter.h"
#include "StringBuilder.h"
#include "tabelas.h"


Simulador* simulador;

// ====================== Funções Auxiliares ======================

// Desbloqueia processos esperando por IO
void desbloquear_processos_io(Simulador* sim) {
    char buf[100];
    PROCESS_HASHMAP_FOREACH(sim->process_map_main, process) {
        if (process->value->state == PROCESS_BLOCKED && 
            process->value->blocked_reason == IO && 
            process_input[0] != '\0') {
            
            desbloquear_processo(sim, process->value);
            snprintf(buf, sizeof(buf), " [Processo %s [%u] desbloqueado por IO.]",
                     process->value->name, process->value->pid);
            strcat(process_output, buf);
        }
    }
}

// Desbloqueia processos com motivo desconhecido aleatoriamente
void desbloquear_processos_aleatorios(Simulador* sim) {
    char buf[100];
    PROCESS_HASHMAP_FOREACH(sim->process_map_main, process) {
        if (process->value->state == PROCESS_BLOCKED && 
            process->value->blocked_reason == UNKNOWN_REASON && 
            (rand() % 4 == 0)) {
            
            desbloquear_processo(sim, process->value);
            snprintf(buf, sizeof(buf), " [Processo %s [%u] desbloqueado aleatoriamente.]",
                     process->value->name, process->value->pid);
            strcat(process_output, buf);
        }
    }
}

// Dessuspende processos da memória secundária
void dessuspender_processos(Simulador* sim) {
    PROCESS_HASHMAP_FOREACH_SAFE(sim->process_map_secondary, entry_var, next_var) {
        if (entry_var->value->state == PROCESS_SUSPENDED && try_swipe(sim, entry_var->value)) {
            char msg[100];
            snprintf(msg, sizeof(msg), " [Processo %s [%u] desuspenso e movido para memória principal.]",
                     entry_var->value->name, entry_var->value->pid);
            printf("%s", msg);
            strcat(process_output, msg);
        }
    }
}

// Escalonador principal
void escalonamento(Simulador* sim) {
    desbloquear_processos_io(sim);
    desbloquear_processos_aleatorios(sim);
    dessuspender_processos(sim);
}

// Tenta escalonar um novo processo
bool tentar_escalonar(Simulador* sim) {
    if (process_queue_is_empty(sim->process_queue)) {
        printf("\n\033[31mNenhum processo pronto para executar.\033[0m\n");
        strcpy(process_output, "Nenhum processo pronto para executar.");
        escalonamento(sim);  // Tenta desbloquear/dessuspender processos
        return false;
    }
    if (sim->current_cycle % 10 == 0) {
        escalonamento(sim);  // Executa escalonamento a cada 10 ciclos
    }

    sim->current_process = process_queue_dequeue(sim->process_queue);
    sim->current_process->state = PROCESS_RUNNING;
    sim->current_process->time_slice_remaining = sim->config.TIME_SLICE;
    reset_tlb_validity(sim->tlb);

    char msg[150];
    snprintf(msg, sizeof(msg), " [Processo %s [%u] escalonado para execução.]",
             sim->current_process->name, sim->current_process->pid);
    printf("%s", msg);
    strcat(process_output, msg);

    return true;
}

// Executa uma instrução do processo atual
void executar_instrucao(Simulador* sim) {
    // Preparar execução
    process_error[0] = '\0';
    process_output[0] = '\0';

    // Obter instrução atual
    Instruction* instructions = sim->current_process->instructions;
    const size_t current_index = sim->current_process->instruction_index;
    const Instruction current_inst = instructions[current_index];

    // Mostrar estado atual
    printf("\n=== Instruction(%zu) Cycle(%u) ===", current_index, sim->current_cycle);
    printf("\nProcesso: %s [%u]", sim->current_process->name, sim->current_process->pid);

    // Tratar instruções de entrada
    if ((current_inst.type == INST_INPUT_N || current_inst.type == INST_INPUT_S) && 
        process_input[0] == '\0') {
        bloquear_processo_atual(sim, IO, 0);
        strcpy(process_output, "Processo bloqueado para esperar input.");
        printf("\n\033[34m%s\033[0m", process_output);
        return;
    }

    // Executar instrução
    simulador = sim;  // Contexto global temporário
    execute(current_index, sim->current_process->pid, instructions);

    // Atualizar estado do processo
    if (sim->current_process) {
        sim->current_process->instruction_index++;
        sim->current_process->time_slice_remaining--;
    }
}

// Verifica condições de término do processo
void verificar_termino_processo(Simulador* sim) {
    // Verificar fim das instruções
    if (sim->current_process->instruction_index >= sim->current_process->instruction_count) {
        terminar_processo(sim, sim->current_process);
        sim->current_process = NULL;
        return;
    }

    // Verificar fim do time slice
    if (sim->current_process->time_slice_remaining <= 0) {
        sim->current_process->state = PROCESS_READY;
        process_queue_enqueue(sim->process_queue, sim->current_process);
        sim->current_process = NULL;
    }
}

// Exibe mensagens de erro/saída
void exibir_mensagens() {
    if (process_error[0] != '\0') {
        printf("\n\033[31mErro: %s\033[0m", process_error);
    } else if (process_output[0] != '\0') {
        printf("\n\033[34m%s\033[0m", process_output);
    }
    fflush(stdout);
}

// Função principal do ciclo de execução
bool proxima_acao(Simulador* sim) {
    sim->current_cycle++;
    sim->important_cycle = false;

    // Gerenciar processo atual
    if (!sim->current_process) {
        return tentar_escalonar(sim);
    }

    // Executar próxima instrução se disponível
    if (sim->current_process->instruction_index < sim->current_process->instruction_count) {
        executar_instrucao(sim);
    }

    // Verificar condições de término
    if (sim->current_process) {
        verificar_termino_processo(sim);
    }

    exibir_mensagens();
    return true;
}
Simulador* create_simulator(const SimulationConfig config) {
    Simulador* sim = malloc(sizeof(Simulador));

    sim->config = config;

    // Inicializa contextos de alocação
    void* main_memory_buffer = malloc(config.MP_SIZE);
    if (!main_memory_buffer) {
        perror("Falha ao alocar memória principal");
        exit(EXIT_FAILURE);
    }
    void* secondary_memory_buffer = malloc(config.MS_SIZE);
    if (!secondary_memory_buffer) {
        perror("Falha ao alocar memória secundária");
        free(main_memory_buffer);
        exit(EXIT_FAILURE);
    }
    sim->main_memory_ctx = nalloc_init(main_memory_buffer, config.MP_SIZE);
    sim->secondary_memory_ctx = nalloc_init(secondary_memory_buffer, config.MS_SIZE);

    // Cria mapa de processos
    sim->process_map_main = process_hashmap_create(sim->main_memory_ctx, 10);
    sim->process_map_secondary = process_hashmap_create(sim->secondary_memory_ctx, 10);

    // Cria fila de processos
    sim->process_queue = process_queue_create(sim->main_memory_ctx);

    // Cria TLB
    sim->tlb = create_tlb(&sim->main_memory_ctx, config.TLB_SIZE);

    sim->current_cycle = 0;

    return sim;
}

void destroy_simulator(Simulador* sim) {
    if (!sim) return;

    // Destroi processos na memória principal
    PROCESS_HASHMAP_FOREACH(sim->process_map_main, entry) {
        terminar_processo(sim, entry->value);
    }
    // Destroi processos na memória secundária
    PROCESS_HASHMAP_FOREACH(sim->process_map_secondary, entry) {
        terminar_processo(sim, entry->value);
    }

    // Destrói TLB
    destroy_tlb(&sim->main_memory_ctx, sim->tlb);

    // Destrói fila de processos
    process_queue_destroy(sim->process_queue);

    // Destrói mapas de processos
    process_hashmap_destroy(sim->process_map_main);
    process_hashmap_destroy(sim->process_map_secondary);

    // Libera contextos de alocação
    free(sim->main_memory_ctx.base_addr);
    free(sim->secondary_memory_ctx.base_addr);
    free(sim);
}

Process* get_process_by_pid(Simulador* simulador, uint32_t pid) {
    Process* process = NULL;

    // Tenta obter o processo da memória principal
    if (process_hashmap_get(simulador->process_map_main, pid, &process)) {
        return process;
    }

    // Se não encontrado, tenta na memória secundária
    if (process_hashmap_get(simulador->process_map_secondary, pid, &process)) {
        return process;
    }

    // Processo não encontrado
    return NULL;
}





// Make json ==========================================================================================================


// Função auxiliar para converter estado do processo para string
const char* process_state_to_str(ProcessState state) {
    switch(state) {
        case PROCESS_RUNNING:   return "Executando";
        case PROCESS_READY:     return "Pronto";
        case PROCESS_SUSPENDED:  return "Suspenso";
        case PROCESS_BLOCKED:   return "Bloqueado";
        default:                return "Desconhecido";
    }
}

char* generate_simulator_json(Simulador* sim) {
    StringBuilder sb;
    sb_init(&sb);
    char buffer[2048];  // Buffer maior para lidar com strings complexas

    // Cabeçalho do JSON com informações básicas
    snprintf(buffer, sizeof(buffer),
        "{\n"
        "\"cycle\": %u,\n"
        "\"important_cycle\": %s,\n"
        "\"main-memory_usage\": %f,\n"
        "\"secondary-memory_usage\": %f,\n"
        "\"last_msg\": \"%s\",\n"
        "\"last_error\": \"%s\",\n",
        sim->current_cycle,
        sim->important_cycle ? "true" : "false",
        (float) nalloc_allocated_size(&sim->main_memory_ctx) / sim->main_memory_ctx.total_size,
        (float) nalloc_allocated_size(&sim->secondary_memory_ctx) / sim->secondary_memory_ctx.total_size,
        process_output,
        process_error
    );
    sb_append(&sb, buffer);

    // Fila de processos if not empty
    if (process_queue_is_empty(sim->process_queue)) {
        sb_append(&sb, "\"process_queue\": [],\n");
    } else {
        sb_append(&sb, "\"process_queue\": [\n");
        bool first_queue = true;
        PROCESS_QUEUE_FOREACH(sim->process_queue, node) {
            if (!first_queue) sb_append(&sb, ",\n");
            first_queue = false;
            snprintf(buffer, sizeof(buffer), "  {\"name\": \"%s\", \"icon\": \"icons/%s.png\"}",
                     node->process->name, node->process->name);
            sb_append(&sb, buffer);
        }
        sb_append(&sb, "\n],\n");
    }

    // Processo atual
    if (sim->current_process) {
        snprintf(buffer, sizeof(buffer),
            "\"current_process\": {\n"
            "  \"name\": \"%s\",\n"
            "  \"icon\": \"icons/%s.png\",\n"
            "  \"remaining-cycles\": %u\n"
            "},\n",
            sim->current_process->name,
            sim->current_process->name,
            sim->current_process->time_slice_remaining
        );
    } else {
        snprintf(buffer, sizeof(buffer), "\"current_process\": null,\n");
    }
    sb_append(&sb, buffer);

    // Lista de processos
    sb_append(&sb, "\"process_list\": [\n");
    bool first_process = true;

    // Processos na memória principal
    if (!process_hashmap_is_empty(sim->process_map_main)) {
        PROCESS_HASHMAP_FOREACH(sim->process_map_main, entry) {
            Process* p = entry->value;

            if (!first_process) sb_append(&sb, ",\n");
            first_process = false;

            snprintf(buffer, sizeof(buffer),
                "  {\n"
                "    \"name\": \"%s\",\n"
                "    \"pid\": %u,\n"
                "    \"state\": \"%s\",\n"
                "    \"page_table\": [\n",
                p->name, p->pid, process_state_to_str(p->state));
            sb_append(&sb, buffer);

            // Entradas da tabela de páginas
            bool first_page = true;
            for (uint32_t i = 0; i < p->page_table->num_entries; i++) {
                PAGE_TABLE_ENTRY* entry = &p->page_table->entries[i];
                if (entry->valid) {
                    if (!first_page) sb_append(&sb, ",\n");
                    first_page = false;

                    uint32_t virt_addr = i * sim->config.PAGE_SIZE;
                    const char* dirty = entry->dirty ? "Sim" : "Não";
                    const char* referenced = entry->referenced ? "Sim" : "Não";

                    snprintf(buffer, sizeof(buffer),
                        "      {\"virtual\": \"0x%04x\", \"real\": \"%p\", \"dirty\": \"%s\", \"referenced\": \"%s\"}",
                        virt_addr, (void*)entry->frame, dirty, referenced);
                    sb_append(&sb, buffer);
                }
            }
            sb_append(&sb, "\n    ]\n  }");
        }
    }


    // Processos na memória secundária
    if (!process_hashmap_is_empty(sim->process_map_secondary)) {
        PROCESS_HASHMAP_FOREACH(sim->process_map_secondary, entry) {
            Process* p = entry->value;

            if (!first_process) sb_append(&sb, ",\n");
            first_process = false;

            snprintf(buffer, sizeof(buffer),
                "  {\n"
                "    \"name\": \"%s\",\n"
                "    \"pid\": %u,\n"
                "    \"state\": \"%s\",\n"
                "    \"page_table\": [\n",
                p->name, p->pid, process_state_to_str(p->state));
            sb_append(&sb, buffer);

            // Entradas da tabela de páginas
            bool first_page = true;
            for (uint32_t i = 0; i < p->page_table->num_entries; i++) {
                PAGE_TABLE_ENTRY* entry = &p->page_table->entries[i];
                if (entry->valid) {
                    if (!first_page) sb_append(&sb, ",\n");
                    first_page = false;

                    uint32_t virt_addr = i * sim->config.PAGE_SIZE;
                    uint32_t phys_addr = (uint32_t)(entry->frame) * sim->config.PAGE_SIZE;
                    const char* dirty = entry->dirty ? "Sim" : "Não";
                    const char* referenced = entry->referenced ? "Sim" : "Não";

                    snprintf(buffer, sizeof(buffer),
                        "      {\"virtual\": \"0x%04x\", \"real\": \"0x%04x\", \"dirty\": \"%s\", \"referenced\": \"%s\"}",
                        virt_addr, phys_addr, dirty, referenced);
                    sb_append(&sb, buffer);
                }
            }
            sb_append(&sb, "\n    ]\n  }");
        }
    }
    sb_append(&sb, "\n],\n");

    // TLB
    if (sim->tlb) {
        sb_append(&sb, "\"tlb\": [");
        bool first_tlb = true;
        for (uint32_t i = 0; i < sim->tlb->size; i++) {
            const TLB_ENTRY* entry = &sim->tlb->entries[i];
            if (!first_tlb) {
                sb_append(&sb, ",");
            }
            first_tlb = false;
            sb_append(&sb, "\n        ");

            const uint32_t virt_addr = entry->page * sim->config.PAGE_SIZE;
            const uint32_t phys_addr = (uint32_t)(entry->frame) * sim->config.PAGE_SIZE;

            snprintf(buffer, sizeof(buffer),
                "{\"index\": %u, \"valid\": %s, \"virtual\": \"0x%04x\", "
                "\"real\": \"0x%04x\", \"last_used\": %u, \"referenced\": %s}",
                i,
                entry->valid ? "true" : "false",
                virt_addr,
                phys_addr,
                entry->last_used,
                entry->referenced ? "true" : "false");
            sb_append(&sb, buffer);
        }
        sb_append(&sb, "\n    ]");
    }
    else {
        sb_append(&sb, "\"tlb\": []\n");
    }
    sb_append(&sb, "\n}");


    return sb_finalize(&sb);
}