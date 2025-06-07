//
// Created by Nathan on 03/06/2025.
//


#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "n.h"
#include "Simulador.h"
#include "process.h"

void inst_create(char *process_name, const size_t pid);
void inst_terminate(const size_t pid);
void inst_mmap(const char *var_name, uintptr_t add_like, int size, const size_t pid);
void inst_print_n(const char *var_name, const size_t pid);
void inst_print_p(const char *var_name, const size_t pid);
void inst_print_s(const char *var_name, const size_t pid);
void inst_assign(const char *var1, const char *var2, const size_t pid);
void inst_assign_var_num(const char *var1, int num, const size_t pid);
void inst_assign_add_num(const char *var1, const char *var2, int num, const size_t pid);
void inst_assign_sub_num(const char *var1, const char *var2, int num, const size_t pid);
void inst_assign_add_var(const char *var1, const char *var2, const char *var3, const size_t pid);
void inst_assign_sub_var(const char *var1, const char *var2, const char *var3, const size_t pid);
void inst_label(const char *label_name, const size_t pid);
void inst_jump(int target_index, const size_t pid);
void inst_jump_eq_varnum(int target_index, const char *var, int num, const size_t pid);
void inst_jump_eq_varvar(int target_index, const char *var1, const char *var2, const size_t pid);
void inst_jump_neq_varnum(int target_index, const char *var, int num, const size_t pid);
void inst_jump_neq_varvar(int target_index, const char *var1, const char *var2, const size_t pid);
void inst_jump_lt_varnum(int target_index, const char *var, int num, const size_t pid);
void inst_jump_lt_varvar(int target_index, const char *var1, const char *var2, const size_t pid);
void inst_jump_gt_varnum(int target_index, const char *var, int num, const size_t pid);
void inst_jump_gt_varvar(int target_index, const char *var1, const char *var2, const size_t pid);
void inst_input_n(const char *var_name, const size_t pid);
void inst_input_s(const char *var_name, int size, const size_t pid);



void execute(const size_t index, const size_t pid, Instruction *instructions) {
    Instruction *inst = &instructions[index];
    switch (inst->type) {
        case INST_CREATE:
            inst_create(inst->args.create.process_name, pid);
            break;
        case INST_TERMINATE:
            inst_terminate(pid);
            break;
        case INST_MMAP:
            inst_mmap(inst->args.mmap.var_name, inst->args.mmap.add_like, inst->args.mmap.size, pid);
            break;
        case INST_PRINT_N:
            inst_print_n(inst->args.print.var_name, pid);
            break;
        case INST_PRINT_P:
            inst_print_p(inst->args.print.var_name, pid);
            break;
        case INST_PRINT_S:
            inst_print_s(inst->args.print.var_name, pid);
            break;
        case INST_ASSIGN:
            inst_assign(inst->args.assign.var1, inst->args.assign.var2, pid);
            break;
        case INST_ASSIGN_VAR_NUM:
            inst_assign_var_num(inst->args.assign_var_num.var1, inst->args.assign_var_num.num, pid);
            break;
        case INST_ASSIGN_ADD_NUM:
            inst_assign_add_num(inst->args.assign_num.var1, inst->args.assign_num.var2, inst->args.assign_num.num, pid);
            break;
        case INST_ASSIGN_SUB_NUM:
            inst_assign_sub_num(inst->args.assign_num.var1, inst->args.assign_num.var2, inst->args.assign_num.num, pid);
            break;
        case INST_ASSIGN_ADD_VAR:
            inst_assign_add_var(inst->args.assign_var.var1, inst->args.assign_var.var2, inst->args.assign_var.var3, pid);
            break;
        case INST_ASSIGN_SUB_VAR:
            inst_assign_sub_var(inst->args.assign_var.var1, inst->args.assign_var.var2, inst->args.assign_var.var3, pid);
            break;
        case INST_LABEL:
            inst_label(inst->args.label.label_name, pid);
            break;
        case INST_JUMP:
            inst_jump(inst->args.jump.index, pid);
            break;
        case INST_JUMP_EQ_VAR_NUM:
            inst_jump_eq_varnum(inst->args.jump_eq_varnum.target.index,
                               inst->args.jump_eq_varnum.var,
                               inst->args.jump_eq_varnum.num,
                               pid);
            break;
        case INST_JUMP_EQ_VAR_VAR:
            inst_jump_eq_varvar(inst->args.jump_eq_varvar.target.index,
                              inst->args.jump_eq_varvar.var1,
                              inst->args.jump_eq_varvar.var2,
                              pid);
            break;
        case INST_JUMP_N_EQ_VAR_NUM:
            inst_jump_neq_varnum(inst->args.jump_neq_varnum.target.index,
                                inst->args.jump_neq_varnum.var,
                                inst->args.jump_neq_varnum.num,
                                pid);
            break;
        case INST_JUMP_N_EQ_VAR_VAR:
            inst_jump_neq_varvar(inst->args.jump_neq_varvar.target.index,
                               inst->args.jump_neq_varvar.var1,
                               inst->args.jump_neq_varvar.var2,
                               pid);
            break;
        case INST_JUMP_LT_VAR_NUM:
            inst_jump_lt_varnum(inst->args.jump_lt_varnum.target.index,
                              inst->args.jump_lt_varnum.var,
                              inst->args.jump_lt_varnum.num,
                              pid);
            break;
        case INST_JUMP_LT_VAR_VAR:
            inst_jump_lt_varvar(inst->args.jump_lt_varvar.target.index,
                             inst->args.jump_lt_varvar.var1,
                             inst->args.jump_lt_varvar.var2,
                             pid);
            break;
        case INST_JUMP_GT_VAR_NUM:
            inst_jump_gt_varnum(inst->args.jump_gt_varnum.target.index,
                              inst->args.jump_gt_varnum.var,
                              inst->args.jump_gt_varnum.num,
                              pid);
            break;
        case INST_JUMP_GT_VAR_VAR:
            inst_jump_gt_varvar(inst->args.jump_gt_varvar.target.index,
                             inst->args.jump_gt_varvar.var1,
                             inst->args.jump_gt_varvar.var2,
                             pid);
            break;

        case INST_INPUT_N:
            inst_input_n(inst->args.input_n.var_name, pid);
            break;
        case INST_INPUT_S:
            inst_input_s(inst->args.input_s.var_name, inst->args.input_s.size, pid);
            break;
        default:
            printf("Instrução desconhecida\n");
            break;
    }
}


// ------------------- ============================== -------------------

#define MAX_MSG_SIZE 256
char process_input[MAX_MSG_SIZE]  =  {0};
char process_output[MAX_MSG_SIZE] =  {0};
char process_error[MAX_MSG_SIZE]  =  {0};

// Função auxiliar para alocar variáveis
bool allocate_variable(Process* p, const char* var_name) {
    return hashmap_put(p->variables_adrr, var_name, 0);
}

// Função auxiliar para ler variáveis
uint32_t read_variable(Process* p, const char* var_name) {
    const char* key = (var_name[0] == '&') ? var_name + 1 : var_name;
    uint32_t addr;

    if (!hashmap_get(p->variables_adrr, key, &addr)) {
        if (!allocate_variable(p, key)) {
            snprintf(process_error, MAX_MSG_SIZE, "Falha ao alocar variavel %s", var_name);
            return 0;
        }
        hashmap_get(p->variables_adrr, key, &addr);
    }

    if (var_name[0] == '&') {
        return addr;
    }

    uint32_t value = 0;
    for (int i = 0; i < 4; i++) {
        int status;
        uint8_t byte = get_mem(simulador, p, addr + i, &status);
        if (status != MEM_ACCESS_OK) {
            snprintf(process_error, MAX_MSG_SIZE, "Erro de acesso a memoria em 0x%x: %s",
                     addr + i, ADDR_STATUS(status));
            return 0;
        }
        value |= (byte << (8 * i));
    }
    return value;
}

// Função auxiliar para escrever variáveis
void write_variable(Process* p, const char* var_name, uint32_t value) {
    const char* key = (var_name[0] == '&') ? var_name + 1 : var_name;
    uint32_t addr;

    if (!hashmap_get(p->variables_adrr, key, &addr)) {
        if (!allocate_variable(p, key)) {
            snprintf(process_error, MAX_MSG_SIZE, "Falha ao alocar variavel %s", var_name);
            return;
        }
        hashmap_get(p->variables_adrr, key, &addr);
    }

    for (int i = 0; i < 4; i++) {
        uint8_t byte = (value >> (8 * i)) & 0xFF;
        int status;
        set_mem(simulador, p, addr + i, byte, &status);
        if (status != MEM_ACCESS_OK) {
            snprintf(process_error, MAX_MSG_SIZE, "Erro de acesso a memoria em 0x%x: %s",
                     addr + i, ADDR_STATUS(status));
            break;
        }
    }
}

// Implementações das instruções ============================================

void inst_create(char *process_name, const size_t pid) {
    // Ler instruções do arquivo
    Instruction *instructions;
    size_t instruction_count;
    char *text_out;
    int text_size;

    snprintf(process_output, MAX_MSG_SIZE, "Criando processo: %s", process_name);

    get_instructions(process_name, &instructions, &instruction_count, &text_out, &text_size);

    criar_processo(simulador, rand(), process_name, instructions, instruction_count, text_out, text_size);
}

void inst_terminate(const size_t pid) {
    snprintf(process_output, MAX_MSG_SIZE, "Terminando processo");
    terminar_processo(simulador, pid);
}

void inst_mmap(const char *var_name, uintptr_t add_like, int size, const size_t pid) {
    Process* p = simulador->current_process;
    const uint32_t page_size = simulador->config.PAGE_SIZE;
    const uint32_t start_page = add_like & ~(page_size - 1);
    const uint32_t end = add_like + size;
    const uint32_t end_page = (end + page_size - 1) & ~(page_size - 1);
    const uint32_t num_pages = (end_page - start_page) / page_size;

    snprintf(process_output, MAX_MSG_SIZE, "Mapeando %d paginas em 0x%lx para %s", num_pages, add_like, var_name);

    for (uint32_t i = 0; i < num_pages; i++) {
        uint32_t addr = start_page + i * page_size;
        if (!allocate_page(simulador, p, addr)) {
            snprintf(process_error, MAX_MSG_SIZE, "Falha ao alocar pagina em 0x%x", addr);
            return;
        }
    }

    if (!hashmap_put(p->variables_adrr, var_name, add_like)) {
        snprintf(process_error, MAX_MSG_SIZE, "Falha ao registrar variavel %s", var_name);
    }
}

void inst_print_n(const char *var_name, const size_t pid) {
    Process* p = simulador->current_process;
    uint32_t value = read_variable(p, var_name);
    if (process_error[0] == '\0') { // Se não houve erro
        snprintf(process_output, MAX_MSG_SIZE, "Printing valor numerico: [%d]", value);
    }
}

void inst_print_p(const char *var_name, const size_t pid) {
    Process* p = simulador->current_process;
    uint32_t value = read_variable(p, var_name);
    if (process_error[0] == '\0') { // Se não houve erro
        snprintf(process_output, MAX_MSG_SIZE, "Printing endereco: [%p]", (void*)(uintptr_t)value);
    }
}

void inst_print_s(const char *var_name, const size_t pid) {
    Process* p = simulador->current_process;
    char str[MAX_MSG_SIZE] = {0};
    size_t len = 0;

    char byte = (char) read_variable(p, var_name);
    while (byte) {
        str[len] = byte;
        str[len + 1] = '\0'; // Garante que a string esteja terminada
        if (len >= MAX_MSG_SIZE - 1) {
            snprintf(process_error, MAX_MSG_SIZE, "String muito longa para imprimir");
            return;
        }
        byte = (char) read_variable(p, var_name + strlen(str));
    }

    if (process_error[0] == '\0') { // Se não houve erro
        snprintf(process_output, MAX_MSG_SIZE, "Printing string: [\"%s\"]", str);
    }
}

void inst_assign(const char *var1, const char *var2, const size_t pid) {
    Process* p = simulador->current_process;
    uint32_t value = read_variable(p, var2);
    if (process_error[0] == '\0') { // Se não houve erro
        snprintf(process_output, MAX_MSG_SIZE, "Atribuindo %s = %s (valor: %d)", var1, var2, value);
    }
    write_variable(p, var1, value);
}

void inst_assign_var_num(const char *var1, int num, const size_t pid) {
    Process* p = simulador->current_process;
    snprintf(process_output, MAX_MSG_SIZE, "Atribuindo %s = %d", var1, num);
    write_variable(p, var1, (uint32_t)num);
}

void inst_assign_add_num(const char *var1, const char *var2, int num, const size_t pid) {
    Process* p = simulador->current_process;
    uint32_t value = read_variable(p, var2) + num;
    snprintf(process_output, MAX_MSG_SIZE, "Atribuindo %s = %s + %d = %d", var1, var2, num, value);
    write_variable(p, var1, value);
}

void inst_assign_sub_num(const char *var1, const char *var2, int num, const size_t pid) {
    Process* p = simulador->current_process;
    uint32_t value = read_variable(p, var2) - num;
    snprintf(process_output, MAX_MSG_SIZE, "Atribuindo %s = %s - %d = %d", var1, var2, num, value);
    write_variable(p, var1, value);
}

void inst_assign_add_var(const char *var1, const char *var2, const char *var3, const size_t pid) {
    Process* p = simulador->current_process;
    uint32_t value2 = read_variable(p, var2);
    uint32_t value3 = read_variable(p, var3);
    uint32_t result = value2 + value3;
    snprintf(process_output, MAX_MSG_SIZE, "Atribuindo %s = %s + %s = %d + %d = %d",
             var1, var2, var3, value2, value3, result);
    write_variable(p, var1, result);
}

void inst_assign_sub_var(const char *var1, const char *var2, const char *var3, const size_t pid) {
    Process* p = simulador->current_process;
    uint32_t value2 = read_variable(p, var2);
    uint32_t value3 = read_variable(p, var3);
    uint32_t result = value2 - value3;
    snprintf(process_output, MAX_MSG_SIZE, "Atribuindo %s = %s - %s = %d - %d = %d",
             var1, var2, var3, value2, value3, result);
    write_variable(p, var1, result);
}

void inst_label(const char *label_name, const size_t pid) {
    // Isso não é exectado
    snprintf(process_output, MAX_MSG_SIZE, "Definindo label: %s", label_name);
}

void inst_jump(int target_index, const size_t pid) {
    Process* p = simulador->current_process;
    snprintf(process_output, MAX_MSG_SIZE, "Jump para instrucao %d", target_index);
    p->instruction_index = target_index - 1;
}

// Macro para saltos condicionais
#define CONDITIONAL_JUMP(cond) do { \
    Process* p = simulador->current_process; \
    if (cond) { \
        snprintf(process_output, MAX_MSG_SIZE, "Condicao verdadeira, jump para instrucao %d", target_index); \
        p->instruction_index = target_index - 1; \
    } else { \
        snprintf(process_output, MAX_MSG_SIZE, "Condicao falsa, continuando"); \
    } \
} while(0)

void inst_jump_eq_varnum(int target_index, const char *var, int num, const size_t pid) {
    uint32_t value = read_variable(simulador->current_process, var);
    CONDITIONAL_JUMP(value == (uint32_t)num);
}

void inst_jump_eq_varvar(int target_index, const char *var1, const char *var2, const size_t pid) {
    uint32_t value1 = read_variable(simulador->current_process, var1);
    uint32_t value2 = read_variable(simulador->current_process, var2);
    CONDITIONAL_JUMP(value1 == value2);
}

void inst_jump_neq_varnum(int target_index, const char *var, int num, const size_t pid) {
    uint32_t value = read_variable(simulador->current_process, var);
    CONDITIONAL_JUMP(value != (uint32_t)num);
}

void inst_jump_neq_varvar(int target_index, const char *var1, const char *var2, const size_t pid) {
    uint32_t value1 = read_variable(simulador->current_process, var1);
    uint32_t value2 = read_variable(simulador->current_process, var2);
    CONDITIONAL_JUMP(value1 != value2);
}

void inst_jump_lt_varnum(int target_index, const char *var, int num, const size_t pid) {
    int32_t value = (int32_t)read_variable(simulador->current_process, var);
    CONDITIONAL_JUMP(value < num);
}

void inst_jump_lt_varvar(int target_index, const char *var1, const char *var2, const size_t pid) {
    int32_t value1 = (int32_t)read_variable(simulador->current_process, var1);
    int32_t value2 = (int32_t)read_variable(simulador->current_process, var2);
    CONDITIONAL_JUMP(value1 < value2);
}

void inst_jump_gt_varnum(int target_index, const char *var, int num, const size_t pid) {
    int32_t value = (int32_t)read_variable(simulador->current_process, var);
    CONDITIONAL_JUMP(value > num);
}

void inst_jump_gt_varvar(int target_index, const char *var1, const char *var2, const size_t pid) {
    int32_t value1 = (int32_t)read_variable(simulador->current_process, var1);
    int32_t value2 = (int32_t)read_variable(simulador->current_process, var2);
    CONDITIONAL_JUMP(value1 > value2);
}

void inst_input_n(const char *var_name, const size_t pid) {
    Process* p = simulador->current_process;
    int value;
    if (sscanf(process_input, "%d", &value) != 1) {
        snprintf(process_error, MAX_MSG_SIZE, "Erro: entrada invalida para inteiro");
        return;
    }
    snprintf(process_output, MAX_MSG_SIZE, "Lendo inteiro: %d", value);
    write_variable(p, var_name, (uint32_t)value);
}

void inst_input_s(const char *var_name, int size, const size_t pid) {
    Process* p = simulador->current_process;
    const uint32_t addr = read_variable(p, var_name);

    // Garante que não ultrapasse o tamanho máximo
    size = (size > MAX_MSG_SIZE) ? MAX_MSG_SIZE : size;

    snprintf(process_output, MAX_MSG_SIZE, "Lendo string: %.*s", size, process_input);

    for (int i = 0; i < size; i++) {
        int status;
        set_mem(simulador, p, addr + i, process_input[i], &status);
        if (status != MEM_ACCESS_OK) {
            snprintf(process_error, MAX_MSG_SIZE, "Erro de acesso a memoria em 0x%x: %s",
                     addr + i, ADDR_STATUS(status));
            break;
        }
        if (process_input[i] == '\0') break;
    }
}

// Testes ======================================================

int main() {
    // Solicitar nome do arquivo
    // char filename[100];
    // printf("Digite o nome do arquivo de instrucoes: ");
    // fgets(filename, sizeof(filename), stdin);
    // // Remover newline
    // size_t len = strlen(filename);
    // if (len > 0 && filename[len-1] == '\n') {
    //     filename[len-1] = '\0';
    // }

    size_t len;
    char filename[] = "test"; // Nome do arquivo de teste

    // Configuração básica do simulador
    SimulationConfig config = {
        .PAGE_SIZE = 4096,
        .MP_SIZE = 1024 * 1024 * 4, // 4 MB
        .MS_SIZE = 1024 * 1024 * 16, // 16 MB
        .TLB_SIZE = 16,
        .TIME_SLICE = 10,
        .SUB_POLICY_TYPE = SUB_POLICY_LRU,
        .BITS_LOGICAL_ADDRESS = 20,
    };
    strcpy(config.FILE_NAME, filename);

    // Inicializar simulador
    Simulador sim = create_simulator(config);
    simulador = &sim;

    // Ler instruções do arquivo
    Instruction *instructions;
    size_t instruction_count;
    char *text_out;
    int text_size;

    get_instructions(filename, &instructions, &instruction_count, &text_out, &text_size);

    // Criar processo principal
    Process* main_process = criar_processo(simulador, 1, "main", instructions, instruction_count, text_out, text_size);
    simulador->current_process = main_process;

    // Loop de execução
    while (simulador->current_process && simulador->current_process->instruction_index < instruction_count) {

        // Preparar para nova instrução
        process_input[0] = '\0';
        process_error[0] = '\0';
        process_output[0] = '\0';

        // Obter instrução atual
        size_t current_index = simulador->current_process->instruction_index;
        Instruction current_inst = instructions[current_index];

        // Mostrar estado atual
        printf("\n=== Instrucao %zu ===", current_index);
        printf("\nProcesso: %s [%u]",
               simulador->current_process->name,
               simulador->current_process->pid);

        // Solicitar entrada do usuário
        if (current_inst.type == INST_INPUT_N || current_inst.type == INST_INPUT_S) {
            printf("\nEntrada necessaria: ");
            fgets(process_input, MAX_MSG_SIZE, stdin);
            // Remover newline
            len = strlen(process_input);
            if (len > 0 && process_input[len-1] == '\n') {
                process_input[len-1] = '\0';
            }
        }

        // Executar instrução
        execute(current_index, simulador->current_process->pid, instructions);

        if (process_error[0] != '\0') {
            printf("\n\033[31mErro: %s\033[0m", process_error);
        }
        else if (process_output[0] != '\0') {
            printf("\n\033[34m%s\033[0m", process_output);
        }

        printf("\nVariables: \n");
        print_hashmap(simulador->current_process->variables_adrr);
        printf("\n");

        // Avançar para próxima instrução (a menos que jump tenha modificado)
        if (simulador->current_process->instruction_index == current_index) {
            simulador->current_process->instruction_index++;
        }
    }

    // Limpeza final
    destroy_simulator(simulador);
    free(instructions);
    if (text_out) free(text_out);

    return 0;
}