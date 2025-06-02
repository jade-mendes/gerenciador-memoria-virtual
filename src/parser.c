//
// Created by nathan on 01/06/2025.
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// Enumeração dos tipos de instrução
typedef enum {
    INST_CREATE,
    INST_TERMINATE,
    INST_MMAP,
    INST_PRINT_N,
    INST_PRINT_P,
    INST_PRINT_S,
    INST_ASSIGN,
    INST_ASSIGN_ADD_NUM,
    INST_ASSIGN_SUB_NUM,
    INST_ASSIGN_ADD_VAR,
    INST_ASSIGN_SUB_VAR,
    INST_LABEL,
    INST_JUMP,
    INST_JUMP_EQ_VAR_NUM,
    INST_JUMP_EQ_VAR_VAR
} InstType;

// Estrutura para argumentos de saltos (com suporte a label/índice)
typedef struct {
    int index;      // Índice numérico (se usado)
    char* label;    // Nome da label (se usado)
} JumpTarget;

// União para argumentos de instruções
typedef union {
    struct { char* process_name; } create;
    // Terminate não tem argumentos
    struct { char* var_name; char* add_like; int size; } mmap;
    struct { char* var_name; } print;
    struct { char* var1; char* var2; } assign;
    struct { char* var1; char* var2; int num; } assign_num;
    struct { char* var1; char* var2; char* var3; } assign_var;
    struct { char* label_name; } label;
    JumpTarget jump;
    struct { JumpTarget target; char* var; int num; } jump_eq_varnum;
    struct { JumpTarget target; char* var1; char* var2; } jump_eq_varvar;
} InstArgs;

// Estrutura de uma instrução
typedef struct {
    InstType type;
    InstArgs args;
} Instruction;

// Tabela de símbolos para labels
typedef struct LabelEntry {
    char* name;
    int index;
    struct LabelEntry* next;
} LabelEntry;

LabelEntry* label_table = NULL;

// Funções auxiliares
void add_label(const char* name, int index) {
    LabelEntry* entry = malloc(sizeof(LabelEntry));
    entry->name = strdup(name);
    entry->index = index;
    entry->next = label_table;
    label_table = entry;
}

int find_label(const char* name) {
    LabelEntry* current = label_table;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current->index;
        }
        current = current->next;
    }
    return -1; // Não encontrado
}

void free_label_table() {
    while (label_table) {
        LabelEntry* next = label_table->next;
        free(label_table->name);
        free(label_table);
        label_table = next;
    }
}

char* trim(char* str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;

    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

// Função principal do parser
Instruction* parse_instructions(FILE* file, int* count) {
    Instruction* instructions = NULL;
    *count = 0;
    int capacity = 0;
    char line[256];
    int current_index = 0;

    while (fgets(line, sizeof(line), file)) {
        char* trimmed = trim(line);
        if (trimmed[0] == ';' || trimmed[0] == '/' || trimmed[0] == '\0') continue;

        // Processar a linha
        if (strncmp(trimmed, "Create(", 7) == 0) {
            char* start = trimmed + 7;
            char* end = strchr(start, ')');
            if (!end) continue;
            *end = '\0';
            char* name = trim(start);

            if (*count >= capacity) {
                capacity = capacity ? capacity * 2 : 16;
                instructions = realloc(instructions, capacity * sizeof(Instruction));
            }
            instructions[*count] = (Instruction){
                .type = INST_CREATE,
                .args.create.process_name = strdup(name)
            };
            (*count)++;
        }
        else if (strcmp(trimmed, "Terminate();") == 0) {
            if (*count >= capacity) {
                capacity = capacity ? capacity * 2 : 16;
                instructions = realloc(instructions, capacity * sizeof(Instruction));
            }
            instructions[*count] = (Instruction){.type = INST_TERMINATE};
            (*count)++;
        }
        else if (strstr(trimmed, "= mmap(")) {
            char* var_end = strstr(trimmed, "=");
            *var_end = '\0';
            char* var_name = trim(trimmed);

            char* start = strstr(var_end + 1, "(") + 1;
            char* end = strchr(start, ')');
            if (!end) continue;
            *end = '\0';

            char* add_like = strtok(start, ",");
            char* size_str = strtok(NULL, ",");
            if (!add_like || !size_str) continue;

            int size = atoi(trim(size_str));
            add_like = trim(add_like);

            if (*count >= capacity) {
                capacity = capacity ? capacity * 2 : 16;
                instructions = realloc(instructions, capacity * sizeof(Instruction));
            }
            instructions[*count] = (Instruction){
                .type = INST_MMAP,
                .args.mmap = {
                    .var_name = strdup(var_name),
                    .add_like = strdup(add_like),
                    .size = size
                }
            };
            (*count)++;
        }
        else if (strncmp(trimmed, "print_", 6) == 0) {
            char type = trimmed[6];
            char* start = strchr(trimmed, '(') + 1;
            char* end = strchr(start, ')');
            if (!end) continue;
            *end = '\0';
            char* var_name = trim(start);

            InstType inst_type;
            if (type == 'n') inst_type = INST_PRINT_N;
            else if (type == 'p') inst_type = INST_PRINT_P;
            else if (type == 's') inst_type = INST_PRINT_S;
            else continue;

            if (*count >= capacity) {
                capacity = capacity ? capacity * 2 : 16;
                instructions = realloc(instructions, capacity * sizeof(Instruction));
            }
            instructions[*count] = (Instruction){
                .type = inst_type,
                .args.print.var_name = strdup(var_name)
            };
            (*count)++;
        }
        else if (strstr(trimmed, "=")) {
            char* var1 = strtok(trimmed, "=");
            char* expr = strtok(NULL, "=");
            if (!var1 || !expr) continue;

            var1 = trim(var1);
            expr = trim(expr);
            expr[strlen(expr) - 1] = '\0'; // Remove ponto e vírgula

            // Verificar tipo de atribuição
            char* op = strpbrk(expr, "+-");
            if (op) {
                char* var2 = strtok(expr, "+-");
                char* operand = strtok(NULL, "+-");
                if (!var2 || !operand) continue;

                var2 = trim(var2);
                operand = trim(operand);

                // Verificar se é número ou variável
                bool is_num = true;
                for (char* p = operand; *p; p++) {
                    if (!isdigit(*p) && *p != '-' && *p != '+') {
                        is_num = false;
                        break;
                    }
                }

                if (is_num) {
                    int num = atoi(operand);
                    if (*count >= capacity) {
                        capacity = capacity ? capacity * 2 : 16;
                        instructions = realloc(instructions, capacity * sizeof(Instruction));
                    }
                    instructions[*count] = (Instruction){
                        .type = (*op == '+') ? INST_ASSIGN_ADD_NUM : INST_ASSIGN_SUB_NUM,
                        .args.assign_num = {
                            .var1 = strdup(var1),
                            .var2 = strdup(var2),
                            .num = num
                        }
                    };
                    (*count)++;
                } else {
                    if (*count >= capacity) {
                        capacity = capacity ? capacity * 2 : 16;
                        instructions = realloc(instructions, capacity * sizeof(Instruction));
                    }
                    instructions[*count] = (Instruction){
                        .type = (*op == '+') ? INST_ASSIGN_ADD_VAR : INST_ASSIGN_SUB_VAR,
                        .args.assign_var = {
                            .var1 = strdup(var1),
                            .var2 = strdup(var2),
                            .var3 = strdup(operand)
                        }
                    };
                    (*count)++;
                }
            } else {
                // Atribuição simples
                if (*count >= capacity) {
                    capacity = capacity ? capacity * 2 : 16;
                    instructions = realloc(instructions, capacity * sizeof(Instruction));
                }
                instructions[*count] = (Instruction){
                    .type = INST_ASSIGN,
                    .args.assign = {
                        .var1 = strdup(var1),
                        .var2 = strdup(expr)
                    }
                };
                (*count)++;
            }
        }
        else if (strncmp(trimmed, "label(", 6) == 0) {
            char* start = trimmed + 6;
            char* end = strchr(start, ')');
            if (!end) continue;
            *end = '\0';
            char* name = trim(start);
            add_label(name, current_index);
        }
        else if (strncmp(trimmed, "jump(", 5) == 0) {
            char* start = trimmed + 5;
            char* end = strchr(start, ')');
            if (!end) continue;
            *end = '\0';
            char* target = trim(start);

            if (*count >= capacity) {
                capacity = capacity ? capacity * 2 : 16;
                instructions = realloc(instructions, capacity * sizeof(Instruction));
            }
            instructions[*count] = (Instruction){
                .type = INST_JUMP,
                .args.jump = {
                    .index = -1,
                    .label = strdup(target)
                }
            };
            (*count)++;
            current_index++;
        }
        else if (strncmp(trimmed, "jump_eq(", 8) == 0) {
            char* start = trimmed + 8;
            char* end = strchr(start, ')');
            if (!end) continue;
            *end = '\0';

            char* target = strtok(start, ",");
            char* var = strtok(NULL, ",");
            char* operand = strtok(NULL, ",");
            if (!target || !var || !operand) continue;

            target = trim(target);
            var = trim(var);
            operand = trim(operand);

            // Verificar se o operando é número ou variável
            bool is_num = true;
            for (char* p = operand; *p; p++) {
                if (!isdigit(*p) && *p != '-' && *p != '+') {
                    is_num = false;
                    break;
                }
            }

            if (*count >= capacity) {
                capacity = capacity ? capacity * 2 : 16;
                instructions = realloc(instructions, capacity * sizeof(Instruction));
            }

            if (is_num) {
                int num = atoi(operand);
                instructions[*count] = (Instruction){
                    .type = INST_JUMP_EQ_VAR_NUM,
                    .args.jump_eq_varnum = {
                        .target = {.index = -1, .label = strdup(target)},
                        .var = strdup(var),
                        .num = num
                    }
                };
            } else {
                instructions[*count] = (Instruction){
                    .type = INST_JUMP_EQ_VAR_VAR,
                    .args.jump_eq_varvar = {
                        .target = {.index = -1, .label = strdup(target)},
                        .var1 = strdup(var),
                        .var2 = strdup(operand)
                    }
                };
            }
            (*count)++;
            current_index++;
        }
    }

    // Resolver labels
    for (int i = 0; i < *count; i++) {
        Instruction* inst = &instructions[i];
        switch (inst->type) {
            case INST_JUMP:
                if (inst->args.jump.label) {
                    int idx = find_label(inst->args.jump.label);
                    if (idx != -1) {
                        inst->args.jump.index = idx;
                        free(inst->args.jump.label);
                        inst->args.jump.label = NULL;
                    }
                }
                break;

            case INST_JUMP_EQ_VAR_NUM:
                if (inst->args.jump_eq_varnum.target.label) {
                    int idx = find_label(inst->args.jump_eq_varnum.target.label);
                    if (idx != -1) {
                        inst->args.jump_eq_varnum.target.index = idx;
                        free(inst->args.jump_eq_varnum.target.label);
                        inst->args.jump_eq_varnum.target.label = NULL;
                    }
                }
                break;

            case INST_JUMP_EQ_VAR_VAR:
                if (inst->args.jump_eq_varvar.target.label) {
                    int idx = find_label(inst->args.jump_eq_varvar.target.label);
                    if (idx != -1) {
                        inst->args.jump_eq_varvar.target.index = idx;
                        free(inst->args.jump_eq_varvar.target.label);
                        inst->args.jump_eq_varvar.target.label = NULL;
                    }
                }
                break;

            default:
                break;
        }
    }

    return instructions;
}

// Função para liberar memória
void free_instructions(Instruction* insts, int count) {
    for (int i = 0; i < count; i++) {
        switch (insts[i].type) {
            case INST_CREATE:
                free(insts[i].args.create.process_name);
                break;
            case INST_MMAP:
                free(insts[i].args.mmap.var_name);
                free(insts[i].args.mmap.add_like);
                break;
            case INST_PRINT_N:
            case INST_PRINT_P:
            case INST_PRINT_S:
                free(insts[i].args.print.var_name);
                break;
            case INST_ASSIGN:
                free(insts[i].args.assign.var1);
                free(insts[i].args.assign.var2);
                break;
            case INST_ASSIGN_ADD_NUM:
            case INST_ASSIGN_SUB_NUM:
                free(insts[i].args.assign_num.var1);
                free(insts[i].args.assign_num.var2);
                break;
            case INST_ASSIGN_ADD_VAR:
            case INST_ASSIGN_SUB_VAR:
                free(insts[i].args.assign_var.var1);
                free(insts[i].args.assign_var.var2);
                free(insts[i].args.assign_var.var3);
                break;
            case INST_JUMP:
                if (insts[i].args.jump.label) free(insts[i].args.jump.label);
                break;
            case INST_JUMP_EQ_VAR_NUM:
                if (insts[i].args.jump_eq_varnum.target.label)
                    free(insts[i].args.jump_eq_varnum.target.label);
                free(insts[i].args.jump_eq_varnum.var);
                break;
            case INST_JUMP_EQ_VAR_VAR:
                if (insts[i].args.jump_eq_varvar.target.label)
                    free(insts[i].args.jump_eq_varvar.target.label);
                free(insts[i].args.jump_eq_varvar.var1);
                free(insts[i].args.jump_eq_varvar.var2);
                break;
            default:
                break;
        }
    }
    free(insts);
    free_label_table();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <arquivo_de_entrada>\n", argv[0]);
        return 1;
    }

    FILE* file = fopen(argv[1], "r");
    if (!file) {
        perror("Erro ao abrir arquivo");
        return 1;
    }

    int count;
    Instruction* instructions = parse_instructions(file, &count);
    fclose(file);

    // Exemplo de uso: imprimir as instruções parseadas
    for (int i = 0; i < count; i++) {
        printf("Instrução %d: ", i);
        switch (instructions[i].type) {
            case INST_CREATE:
                printf("CREATE(%s)\n", instructions[i].args.create.process_name);
                break;
            case INST_TERMINATE:
                printf("TERMINATE\n");
                break;
            case INST_MMAP:
                printf("MMAP(%s, %s, %d)\n",
                       instructions[i].args.mmap.var_name,
                       instructions[i].args.mmap.add_like,
                       instructions[i].args.mmap.size);
                break;
            case INST_PRINT_N:
                printf("PRINT_N(%s)\n", instructions[i].args.print.var_name);
                break;
            case INST_PRINT_P:
                printf("PRINT_P(%s)\n", instructions[i].args.print.var_name);
                break;
            case INST_PRINT_S:
                printf("PRINT_S(%s)\n", instructions[i].args.print.var_name);
                break;
            case INST_ASSIGN:
                printf("ASSIGN(%s = %s)\n",
                       instructions[i].args.assign.var1,
                       instructions[i].args.assign.var2);
                break;
            // ... outros casos
        }
    }

    free_instructions(instructions, count);
    return 0;
}