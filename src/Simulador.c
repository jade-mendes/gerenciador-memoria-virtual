//
// Created by natha on 05/06/2025.
//


#include "Simulador.h"
#include "process.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tabelas.h"


Simulador* simulador;

void proxima_acao(Simulador* sim) {
    if (!sim->current_process) {
        // Escalona novo processo se disponível
        if (!process_queue_is_empty(sim->process_queue)) {
            sim->current_process = process_queue_dequeue(sim->process_queue);
            sim->current_process->state = PROCESS_RUNNING;
            sim->current_process->time_slice_remaining = sim->config.TIME_SLICE;
        }
        return;
    }

    // Executa próxima instrução
    if (sim->current_process->instruction_index < sim->current_process->instruction_count) {

        // Simula execução da instrução

        sim->current_process->instruction_index++;
        sim->current_process->time_slice_remaining--;
    }

    // Verifica se o processo terminou
    if (sim->current_process->instruction_index >= sim->current_process->instruction_count) {
        terminar_processo(sim, sim->current_process->pid);
        sim->current_process = NULL;
        return;
    }

    // Verifica fim do time slice
    if (sim->current_process->time_slice_remaining <= 0) {
        sim->current_process->state = PROCESS_READY;
        process_queue_enqueue(sim->process_queue, sim->current_process);
        sim->current_process = NULL;
    }
}

Simulador create_simulator(const SimulationConfig config) {
    Simulador sim = {0};

    sim.config = config;

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
    sim.main_memory_ctx = nalloc_init(main_memory_buffer, config.MP_SIZE);
    sim.secondary_memory_ctx = nalloc_init(secondary_memory_buffer, config.MS_SIZE);

    // Cria mapa de processos
    sim.process_map_main = process_hashmap_create(sim.main_memory_ctx, 10);
    sim.process_map_secondary = process_hashmap_create(sim.secondary_memory_ctx, 10);

    // Cria fila de processos
    sim.process_queue = process_queue_create(sim.main_memory_ctx);

    // Cria TLB
    sim.tlb = create_tlb(&sim.main_memory_ctx, config.TLB_SIZE);

    return sim;
}

void destroy_simulator(Simulador* sim) {
    if (!sim) return;

    // Destrói TLB
    destroy_tlb(&sim->main_memory_ctx, sim->tlb);

    // Destrói fila de processos
    process_queue_destroy(sim->process_queue);

    // Destroi processos na memória principal
    PROCESS_HASHMAP_FOREACH(sim->process_map_main, entry) {
        destroy_process(entry->value, &sim->main_memory_ctx);
    }
    // Destroi processos na memória secundária
    PROCESS_HASHMAP_FOREACH(sim->process_map_secondary, entry) {
        destroy_process(entry->value, &sim->secondary_memory_ctx);
    }

    // Destrói mapas de processos
    process_hashmap_destroy(sim->process_map_main);
    process_hashmap_destroy(sim->process_map_secondary);

    // Libera contextos de alocação
    free(sim->main_memory_ctx.base_addr);
    free(sim->secondary_memory_ctx.base_addr);
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

// =========================




// int main() {
//     printf("Iniciando testes do sistema de memória virtual...\n");
//
//     // Configuração de teste
//     const size_t MEM_SIZE = 1024 * 32;   // 32KB
//     const uint32_t PAGE_SIZE = 4096;     // 4KB
//     const uint32_t TLB_SIZE = 4;         // 4 entradas
//
//     // Inicializa simulador
//     Simulador sim = create_simulator((SimulationConfig){
//         .PAGE_SIZE = PAGE_SIZE,
//         .MP_SIZE = MEM_SIZE,
//         .MS_SIZE = MEM_SIZE * 16,
//         .TLB_SIZE = TLB_SIZE,
//         .TIME_SLICE = 10, // 10 unidades de tempo
//         .BITS_LOGICAL_ADDRESS = 16, // 12 bits para endereços lógicos
//         .SUB_POLICY_TYPE = SUB_POLICY_CLOCK,
//         .FILE_NAME = "test_file"
//     });
//
//     // Cria processo de teste
//     Process* proc = criar_processo(
//         &sim,
//         1, // PID
//         "ProcessoTeste",
//         NULL, // Instruções não são usadas neste teste
//         0, // Contagem de instruções
//         NULL, // Textos não são usados neste teste
//         0 // Tamanho de textos não é usado neste teste
//     );
//
//     //nalloc_print_memory(&sim.main_memory_ctx);
//     printf("\n=== Teste 1: Alocação básica de página ===\n");
//     const uint32_t virt_addr = 0x4000;
//     bool alloc_success = allocate_page(&sim, proc, virt_addr);
//     printf("Alocação em 0x%08X: %s\n", virt_addr, alloc_success ? "SUCESSO" : "FALHA");
//     assert(alloc_success);
//
//     // Verifica se a entrada na tabela de páginas está correta
//     uint32_t page_num = virt_addr / PAGE_SIZE;
//     assert(page_num < proc->page_table->num_entries);
//     assert(proc->page_table->entries[page_num].valid);
//     assert(proc->page_table->entries[page_num].frame != 0);
//
//     //nalloc_print_memory(&sim.main_memory_ctx);
//     printf("\n=== Teste 2: Escrita e leitura de memória ===\n");
//     int status;
//     const uint8_t test_value = 0xAB;
//     const uint32_t test_offset = 0x100;
//
//     // Escreve valor
//     set_mem(&sim, proc, virt_addr + test_offset, test_value, &status);
//     printf("Escrita em 0x%08X: status: %s\n", virt_addr + test_offset, ADDR_STATUS(status));
//     assert(status == MEM_ACCESS_OK);
//
//     // Verifica escrita
//     uint8_t read_value = get_mem(&sim, proc, virt_addr + test_offset, &status);
//     printf("Leitura em 0x%08X: valor=0x%02X, status=%s\n",
//            virt_addr + test_offset, read_value, ADDR_STATUS(status));
//     assert(status == MEM_ACCESS_OK);
//     assert(read_value == test_value);
//
//     // Verifica se bits de referência e modificação foram setados
//     assert(proc->page_table->entries[page_num].referenced);
//     assert(proc->page_table->entries[page_num].dirty);
//
//     printf("\n=== Teste 3: Verificação da TLB ===\n");
//     uintptr_t frame_addr;
//     bool tlb_hit = tlb_lookup(sim.tlb, page_num, &frame_addr);
//     printf("TLB lookup: %s\n", tlb_hit ? "HIT" : "MISS");
//     assert(tlb_hit);
//     assert(frame_addr == proc->page_table->entries[page_num].frame);
//
//     printf("\n=== Teste 4: Acesso a página não alocada ===\n");
//     const uint32_t invalid_addr = 0x8000;
//     read_value = get_mem(&sim, proc, invalid_addr, &status);
//     printf("Tentativa de leitura em 0x%08X: status=%s\n", invalid_addr, ADDR_STATUS(status));
//     assert(status == MEM_ACCESS_PAGE_NOT_ALLOCATED);
//
//     printf("\n=== Teste 5: Expansão automática da tabela de páginas ===\n");
//     const uint32_t high_addr = 0x100000; // 1MB
//     alloc_success = allocate_page(&sim, proc, high_addr);
//     printf("Alocação em 0x%08X: %s\n", high_addr, alloc_success ? "SUCESSO" : "FALHA");
//     assert(alloc_success);
//
//     uint32_t high_page_num = high_addr / PAGE_SIZE;
//     assert(high_page_num < proc->page_table->num_entries);
//     assert(proc->page_table->entries[high_page_num].valid);
//
//     printf("\n=== Teste 6: Desalocação de página ===\n");
//     deallocate_page(&sim, proc, virt_addr);
//     read_value = get_mem(&sim, proc, virt_addr, &status);
//     printf("Tentativa de leitura após desalocação: status=%s\n", ADDR_STATUS(status));
//     assert(status == MEM_ACCESS_PAGE_NOT_ALLOCATED);
//
//     // Verifica se entrada foi invalidada na TLB
//     tlb_hit = tlb_lookup(sim.tlb, page_num, &frame_addr);
//     printf("TLB lookup após desalocação: %s\n", tlb_hit ? "HIT" : "MISS");
//     assert(!tlb_hit);
//
//     printf("\n=== Teste 7: Reset da TLB ===\n");
//     reset_tlb_validity(sim.tlb);
//     tlb_hit = tlb_lookup(sim.tlb, high_page_num, &frame_addr);
//     printf("TLB lookup após reset: %s\n", tlb_hit ? "HIT" : "MISS");
//     assert(!tlb_hit);
//
//     nalloc_print_memory(&sim.main_memory_ctx);
//     // Limpeza
//     destroy_simulator(&sim);
//     nalloc_print_memory(&sim.main_memory_ctx);
//
//     printf("\nTodos os testes passaram com sucesso!\n");
//     return EXIT_SUCCESS;
// }