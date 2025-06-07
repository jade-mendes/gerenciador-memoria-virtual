//
// Created by natha on 05/06/2025.
//


#include "Simulador.h"
#include "process.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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

// =========================

// Funções de gerenciamento da TLB
bool tlb_lookup(TLB* tlb, const uint32_t page, uintptr_t* frame) {
    for (uint32_t i = 0; i < tlb->size; i++) {
        if (tlb->entries[i].valid && tlb->entries[i].page == page) {
            *frame = tlb->entries[i].frame;
            tlb->entries[i].last_used = tlb->counter++;
            return true;
        }
    }
    return false;
}

void tlb_update(TLB* tlb, const uint32_t page, const uintptr_t frame) {
    uint32_t lru_index = 0;
    uint64_t min_used = UINT64_MAX;
    bool found_invalid = false;

    for (uint32_t i = 0; i < tlb->size; i++) {
        if (!tlb->entries[i].valid) {
            lru_index = i;
            found_invalid = true;
            break;
        }
        if (tlb->entries[i].last_used < min_used) {
            min_used = tlb->entries[i].last_used;
            lru_index = i;
        }
    }

    tlb->entries[lru_index].page = page;
    tlb->entries[lru_index].frame = frame;
    tlb->entries[lru_index].valid = true;
    tlb->entries[lru_index].last_used = tlb->counter++;
}

void tlb_invalidate_entry(const TLB* tlb, const uint32_t page) {
    for (uint32_t i = 0; i < tlb->size; i++) {
        if (tlb->entries[i].valid && tlb->entries[i].page == page) {
            tlb->entries[i].valid = false;
            break;
        }
    }
}

void reset_tlb_validity(const TLB* tlb) {
    for (uint32_t i = 0; i < tlb->size; i++) {
        tlb->entries[i].valid = false;
    }
}

// Funções de acesso à memória
uint8_t get_mem(const Simulador* s, Process* p, const uint32_t virt_addr, int* out_status) {
    uint32_t page_num = virt_addr / s->config.PAGE_SIZE;
    const uint32_t offset = virt_addr % s->config.PAGE_SIZE;

    uintptr_t frame_addr;
    bool tlb_hit = tlb_lookup(s->tlb, page_num, &frame_addr);

    if (page_num >= p->page_table->num_entries) {
        *out_status = MEM_ACCESS_PAGE_NOT_ALLOCATED;
        return 0;
    }
    PAGE_TABLE_ENTRY* entry = &p->page_table->entries[page_num];

    if (!entry->valid) {
        *out_status = MEM_ACCESS_PAGE_NOT_ALLOCATED;
        return 0;
    }

    if (!tlb_hit) {
        frame_addr = entry->frame;
        tlb_update(s->tlb, page_num, frame_addr);
    }

    entry->referenced = true;

    const uint8_t* physical_addr = (uint8_t*)(frame_addr + offset);
    *out_status = MEM_ACCESS_OK;
    return *physical_addr;
}

void set_mem(const Simulador* s, Process* p, const uint32_t virt_addr, const uint8_t value, int* out_status) {
    uint32_t page_num = virt_addr / s->config.PAGE_SIZE;
    uint32_t offset = virt_addr % s->config.PAGE_SIZE;

    uintptr_t frame_addr;
    bool tlb_hit = tlb_lookup(s->tlb, page_num, &frame_addr);

    if (page_num >= p->page_table->num_entries) {
        *out_status = MEM_ACCESS_PAGE_NOT_ALLOCATED;
        return;
    }
    PAGE_TABLE_ENTRY* entry = &p->page_table->entries[page_num];

    if (!entry->valid) {
        *out_status = MEM_ACCESS_PAGE_NOT_ALLOCATED;
        return;
    }

    if (!tlb_hit) {
        frame_addr = entry->frame;
        tlb_update(s->tlb, page_num, frame_addr);
    }

    entry->referenced = true;
    entry->dirty = true;

    uint8_t* physical_addr = (uint8_t*)(frame_addr + offset);
    *physical_addr = value;
    *out_status = MEM_ACCESS_OK;
}

// Funções de gerenciamento de páginas
bool allocate_page(const Simulador* s, Process* p, uintptr_t virt_addr) {
    uint32_t page_num = virt_addr / s->config.PAGE_SIZE;

    if (page_num < p->page_table->num_entries && p->page_table->entries[page_num].valid) {
        return true; // Página já alocada
    }

    uintptr_t frame_addr = (uintptr_t) nalloc_alloc(&s->main_memory_ctx, s->config.PAGE_SIZE);
    if (!frame_addr) {
        return false; // Falha na alocação de memória
    }

    if (page_num >= p->page_table->num_entries) {
        uint32_t new_num_entries = page_num + 1;
        PAGE_TABLE_ENTRY* new_entries = nalloc_realloc(
            &s->main_memory_ctx,
            p->page_table->entries,
            new_num_entries * sizeof(PAGE_TABLE_ENTRY)
        );

        if (!new_entries) {
            nalloc_free(&s->main_memory_ctx, (void*) frame_addr);
            return false; // Falha na realocação de memória
        }

        for (uint32_t i = p->page_table->num_entries; i < new_num_entries; i++) {
            new_entries[i].valid = false;
            new_entries[i].frame = INVALID_FRAME;
            new_entries[i].dirty = false;
            new_entries[i].referenced = false;
        }

        p->page_table->entries = new_entries;
        p->page_table->num_entries = new_num_entries;
    }

    p->page_table->entries[page_num].valid = true;
    p->page_table->entries[page_num].frame = frame_addr;
    p->page_table->entries[page_num].dirty = false;
    p->page_table->entries[page_num].referenced = false;

    return true;
}

void deallocate_page(Simulador* s, Process* p, uint32_t virt_addr) {
    uint32_t page_num = virt_addr / s->config.PAGE_SIZE;

    if (page_num >= p->page_table->num_entries || !p->page_table->entries[page_num].valid) {
        return;
    }

    void* frame_addr = (void*) p->page_table->entries[page_num].frame;

    nalloc_free(&s->main_memory_ctx, frame_addr);
    p->page_table->entries[page_num].valid = false;
    tlb_invalidate_entry(s->tlb, page_num);
}

// Funções de criação/destruição das estruturas
PAGE_TABLE* create_page_table(const NallocContext* ctx) {
    PAGE_TABLE* pt = nalloc_alloc(ctx, sizeof(PAGE_TABLE));
    if (!pt) return NULL;

    pt->num_entries = 0;
    pt->entries = NULL;
    return pt;
}

void destroy_page_table(const NallocContext* ctx, PAGE_TABLE* pt) {
    if (pt->entries) {
        nalloc_free(ctx, pt->entries);
    }
    nalloc_free(ctx, pt);
}

TLB* create_tlb(NallocContext* ctx, const uint32_t size) {
    TLB* tlb = nalloc_alloc(ctx, sizeof(TLB));
    if (!tlb) return NULL;

    tlb->size = size;
    tlb->counter = 0;
    tlb->entries = (TLB_ENTRY*)nalloc_alloc(ctx, size * sizeof(TLB_ENTRY));

    if (!tlb->entries) {
        nalloc_free(ctx, tlb);
        return NULL;
    }

    for (uint32_t i = 0; i < size; i++) {
        tlb->entries[i].valid = false;
    }

    return tlb;
}

void destroy_tlb(const NallocContext* ctx, TLB* tlb) {
    if (tlb->entries) {
        nalloc_free(ctx, tlb->entries);
    }
    nalloc_free(ctx, tlb);
}

// Função para destruir páginas de um processo
void destroy_process_pages(Simulador* s, Process* p) {
    for (uint32_t i = 0; i < p->page_table->num_entries; i++) {
        if (p->page_table->entries[i].valid) {
            deallocate_page(s, p, i * s->config.PAGE_SIZE);
        }
    }
}



int main() {
    printf("Iniciando testes do sistema de memória virtual...\n");

    // Configuração de teste
    const size_t MEM_SIZE = 1024 * 1024; // 1MB
    const uint32_t PAGE_SIZE = 4096;     // 4KB
    const uint32_t TLB_SIZE = 4;         // 4 entradas

    // Inicializa simulador
    Simulador sim = create_simulator((SimulationConfig){
        .PAGE_SIZE = PAGE_SIZE,
        .MP_SIZE = MEM_SIZE,
        .MS_SIZE = MEM_SIZE * 16,
        .TLB_SIZE = TLB_SIZE,
        .TIME_SLICE = 10, // 10 unidades de tempo
        .BITS_LOGICAL_ADDRESS = 12, // 12 bits para endereços lógicos
        .SUB_POLICY_TYPE = SUB_POLICY_CLOCK,
        .FILE_NAME = "test_file"
    });

    // Cria processo de teste
    Process* proc = criar_processo(&sim, 1, "ProcessoTeste", NULL, 0);

    //nalloc_print_memory(&sim.main_memory_ctx);
    printf("\n=== Teste 1: Alocação básica de página ===\n");
    const uint32_t virt_addr = 0x4000;
    bool alloc_success = allocate_page(&sim, proc, virt_addr);
    printf("Alocação em 0x%08X: %s\n", virt_addr, alloc_success ? "SUCESSO" : "FALHA");
    assert(alloc_success);

    // Verifica se a entrada na tabela de páginas está correta
    uint32_t page_num = virt_addr / PAGE_SIZE;
    assert(page_num < proc->page_table->num_entries);
    assert(proc->page_table->entries[page_num].valid);
    assert(proc->page_table->entries[page_num].frame != 0);

    //nalloc_print_memory(&sim.main_memory_ctx);
    printf("\n=== Teste 2: Escrita e leitura de memória ===\n");
    int status;
    const uint8_t test_value = 0xAB;
    const uint32_t test_offset = 0x100;

    // Escreve valor
    set_mem(&sim, proc, virt_addr + test_offset, test_value, &status);
    printf("Escrita em 0x%08X: status %d\n", virt_addr + test_offset, status);
    assert(status == MEM_ACCESS_OK);

    // Verifica escrita
    uint8_t read_value = get_mem(&sim, proc, virt_addr + test_offset, &status);
    printf("Leitura em 0x%08X: valor=0x%02X, status=%d\n",
           virt_addr + test_offset, read_value, status);
    assert(status == MEM_ACCESS_OK);
    assert(read_value == test_value);

    // Verifica se bits de referência e modificação foram setados
    assert(proc->page_table->entries[page_num].referenced);
    assert(proc->page_table->entries[page_num].dirty);

    printf("\n=== Teste 3: Verificação da TLB ===\n");
    uintptr_t frame_addr;
    bool tlb_hit = tlb_lookup(sim.tlb, page_num, &frame_addr);
    printf("TLB lookup: %s\n", tlb_hit ? "HIT" : "MISS");
    assert(tlb_hit);
    assert(frame_addr == proc->page_table->entries[page_num].frame);

    printf("\n=== Teste 4: Acesso a página não alocada ===\n");
    const uint32_t invalid_addr = 0x8000;
    read_value = get_mem(&sim, proc, invalid_addr, &status);
    printf("Tentativa de leitura em 0x%08X: status=%d\n", invalid_addr, status);
    assert(status == MEM_ACCESS_PAGE_NOT_ALLOCATED);

    printf("\n=== Teste 5: Expansão automática da tabela de páginas ===\n");
    const uint32_t high_addr = 0x100000; // 1MB
    alloc_success = allocate_page(&sim, proc, high_addr);
    printf("Alocação em 0x%08X: %s\n", high_addr, alloc_success ? "SUCESSO" : "FALHA");
    assert(alloc_success);

    uint32_t high_page_num = high_addr / PAGE_SIZE;
    assert(high_page_num < proc->page_table->num_entries);
    assert(proc->page_table->entries[high_page_num].valid);

    printf("\n=== Teste 6: Desalocação de página ===\n");
    deallocate_page(&sim, proc, virt_addr);
    read_value = get_mem(&sim, proc, virt_addr, &status);
    printf("Tentativa de leitura após desalocação: status=%d\n", status);
    assert(status == MEM_ACCESS_PAGE_NOT_ALLOCATED);

    // Verifica se entrada foi invalidada na TLB
    tlb_hit = tlb_lookup(sim.tlb, page_num, &frame_addr);
    printf("TLB lookup após desalocação: %s\n", tlb_hit ? "HIT" : "MISS");
    assert(!tlb_hit);

    printf("\n=== Teste 7: Reset da TLB ===\n");
    reset_tlb_validity(sim.tlb);
    tlb_hit = tlb_lookup(sim.tlb, high_page_num, &frame_addr);
    printf("TLB lookup após reset: %s\n", tlb_hit ? "HIT" : "MISS");
    assert(!tlb_hit);

    // Limpeza
    destroy_page_table(&sim.main_memory_ctx, proc->page_table);
    destroy_tlb(&sim.main_memory_ctx, sim.tlb);

    printf("\nTodos os testes passaram com sucesso!\n");
    return EXIT_SUCCESS;
}