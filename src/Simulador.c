//
// Created by natha on 05/06/2025.
//


#include "Simulador.h"
#include "process.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "n_interpreter.h"
#include "tabelas.h"


Simulador* simulador;

void escalonamento(Simulador* sim) {
    char buf[100];
    // verificar se tem coisa para desbloquear
    PROCESS_HASHMAP_FOREACH(sim->process_map_main, process) {
        if (process->value->state == PROCESS_BLOCKED && process->value->blocked_reason == IO) {
            // Desbloqueia o processo se houver entrada do usuário
            if (process_input[0] != '\0') {
                desbloquear_processo(sim, process->value);

                sprintf(buf, "\nProcesso %s [%u] desbloqueado por IO", process->value->name, process->value->pid);
                strcat(process_output, buf);
            }
        }
        else if (process->value->state == PROCESS_BLOCKED && process->value->blocked_reason == UNKNOWN_REASON) {
            if (rand() % 4 == 0) {
                // Simula desbloqueio aleatório
                desbloquear_processo(sim, process->value);

                sprintf(buf, "\nProcesso %s [%u] desbloqueado aleatoriamente", process->value->name, process->value->pid);
                strcat(process_output, buf);
            }
        }
    }

    // Verifica se há processos suspensos para desuspender
    PROCESS_HASHMAP_FOREACH_SAFE(sim->process_map_secondary, entry_var, next_var) {
        if (entry_var->value->state == PROCESS_SUSPENDED) {
            // Desuspende o processo
            if (try_swipe(sim, entry_var->value)) {
                // Se a troca foi bem-sucedida, remove do mapa de secundária
                printf("\nProcesso %s [%u] desuspenso e movido para memória principal", entry_var->value->name, entry_var->value->pid);
                sprintf(process_output, "Processo %s [%u] desuspenso e movido para memória principal", entry_var->value->name, entry_var->value->pid);
            }
        }
    }
}


bool proxima_acao(Simulador* sim) {
    sim->current_cycle++;
    if (!sim->current_process) {
        // Escalona novo processo se disponível
        if (!process_queue_is_empty(sim->process_queue)) {
            sim->current_process = process_queue_dequeue(sim->process_queue);
            sim->current_process->state = PROCESS_RUNNING;
            sim->current_process->time_slice_remaining = sim->config.TIME_SLICE;
            reset_tlb_validity(sim->tlb);

            printf("\n\033[32mProcesso %s [%u] escalonado para execução.\033[0m\n",
                   sim->current_process->name, sim->current_process->pid);
            sprintf(process_output, "Processo %s [%u] escalonado para execução.",
                   sim->current_process->name, sim->current_process->pid);

            return true;
        }

        // Se não há processos prontos, termina a simulação
        printf("\n\033[31mNenhum processo pronto para executar. \033[0m\n");
        sprintf(process_output, "Nenhum processo pronto para executar. ");
        escalonamento(sim);

        return false;
    }

    // Executa próxima instrução
    if (sim->current_process->instruction_index < sim->current_process->instruction_count) {

        // Preparar para nova instrução
        process_error[0] = '\0';
        process_output[0] = '\0';

        // Obter instrução atual
        Instruction* instructions = sim->current_process->instructions;
        const size_t current_index = sim->current_process->instruction_index;
        const Instruction current_inst = instructions[current_index];

        // Mostrar estado atual
        printf("\n=== Instruction(%zu) Cycle(%u) ===", current_index, sim->current_cycle);
        printf("\nProcesso: %s [%u]", sim->current_process->name, sim->current_process->pid);

        // Solicitar entrada do usuário
        if ((current_inst.type == INST_INPUT_N || current_inst.type == INST_INPUT_S) && process_input[0] == '\0') {
            bloquear_processo_atual(sim, IO, 0);
            sprintf(process_output, "Processo bloqueado para esperar input.");
            printf("\n\033[34m%s\033[0m", process_output);
            fflush(stdout);

            return true;
        }

        // Executar instrução
        simulador = sim;
        execute(current_index, sim->current_process->pid, instructions);

        sim->current_process->instruction_index++;
        sim->current_process->time_slice_remaining--;
    }
    else {
        // Se não há mais instruções, termina o processo
        terminar_processo(sim, sim->current_process);
        sim->current_process = NULL;
    }

    // Verifica fim do time slice
    if (sim->current_process && sim->current_process->time_slice_remaining <= 0) {
        sim->current_process->state = PROCESS_READY;
        process_queue_enqueue(sim->process_queue, sim->current_process);
        sim->current_process = NULL;
    }



    // char* json = generate_simulator_json(sim);
    // if (json) {
    //     printf("\n\033[32m%s\033[0m\n", json);
    //     free(json);
    // }

    if (process_error[0] != '\0') {
        printf("\n\033[31mErro: %s\033[0m", process_error);
    }
    else if (process_output[0] != '\0') {
        printf("\n\033[34m%s\033[0m", process_output);
    }

    fflush(stdout);
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

// Implementação do StringBuilder para construção eficiente de JSON
typedef struct {
    char* data;
    size_t length;
    size_t capacity;
} StringBuilder;

static void sb_init(StringBuilder* sb) {
    sb->data = NULL;
    sb->length = 0;
    sb->capacity = 0;
}

static void sb_append(StringBuilder* sb, const char* str) {
    size_t len = strlen(str);
    if (sb->length + len + 1 > sb->capacity) {
        size_t new_capacity = sb->capacity == 0 ? 1024 : sb->capacity * 2;
        while (sb->length + len + 1 > new_capacity) {
            new_capacity *= 2;
        }
        char* new_data = realloc(sb->data, new_capacity);
        if (!new_data) return;
        sb->data = new_data;
        sb->capacity = new_capacity;
    }
    memcpy(sb->data + sb->length, str, len);
    sb->length += len;
    sb->data[sb->length] = '\0';
}

static char* sb_finalize(StringBuilder* sb) {
    char* result = sb->data;
    sb_init(sb);
    return result;
}

// Função auxiliar para converter estado do processo para string
static const char* process_state_to_str(ProcessState state) {
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
        "\"main-memory_usage\": %f,\n"
        "\"secondary-memory_usage\": %f,\n"
        "\"last_msg\": \"%s\",\n",
        sim->current_cycle,
        (float) nalloc_allocated_size(&sim->main_memory_ctx) / sim->main_memory_ctx.total_size,
        (float) nalloc_allocated_size(&sim->secondary_memory_ctx) / sim->secondary_memory_ctx.total_size,
        process_error[0] == '\0' ? process_output : process_error
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
            "  \"last_msg\": \"%s\"\n"
            "},\n",
            sim->current_process->name,
            sim->current_process->name,
            process_error[0] == '\0' ? process_output : process_error
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
        sb_append(&sb, "\"tlb\": [\n");
        bool first_tlb = true;
        for (uint32_t i = 0; i < sim->tlb->size; i++) {
            TLB_ENTRY* entry = &sim->tlb->entries[i];
            if (entry->valid) {
                if (!first_tlb) sb_append(&sb, ",\n");
                first_tlb = false;

                uint32_t virt_addr = entry->page * sim->config.PAGE_SIZE;
                uint32_t phys_addr = (uint32_t)(entry->frame) * sim->config.PAGE_SIZE;

                // Flag da TLB
                const char* referenced = entry->valid ? "Sim" : "Não";
                snprintf(buffer, sizeof(buffer),
                    "  {\"virtual\": \"0x%04x\", \"real\": \"0x%04x\", "
                    "\"last_used\": \"%lu\", \"referenced\": \"%s\"}",
                    virt_addr, phys_addr, entry->last_used, referenced);
                sb_append(&sb, buffer);
            }
        }
        sb_append(&sb, "\n]\n}");
    }
    else {
        sb_append(&sb, "\"tlb\": [],\n");
    }


    return sb_finalize(&sb);
}