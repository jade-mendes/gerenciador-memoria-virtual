#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>

#include  "n.h"

// Tabela de símbolos para labels
typedef struct LabelEntry {
    char name[MAX_NAME_LEN];
    int index;
    struct LabelEntry* next;
} LabelEntry;

LabelEntry* label_table = NULL;

// Funções auxiliares
void add_label(const char* name, int index) {
    LabelEntry* entry = malloc(sizeof(LabelEntry));
    strncpy(entry->name, name, MAX_NAME_LEN - 1);
    entry->name[MAX_NAME_LEN - 1] = '\0';
    entry->index = index;
    entry->next = label_table;
    label_table = entry;
}

int find_label(const char* name) {
    const LabelEntry* current = label_table;
    char search_name[MAX_NAME_LEN];
    strncpy(search_name, name, MAX_NAME_LEN - 1);
    search_name[MAX_NAME_LEN - 1] = '\0';

    while (current) {
        if (strcmp(current->name, search_name) == 0) {
            return current->index;
        }
        current = current->next;
    }
    return -1; // Não encontrado
}

void free_label_table() {
    while (label_table) {
        LabelEntry* next = label_table->next;
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

// Copia string garantindo limite de tamanho
void safe_strcpy(char* dest, const char* src) {
    strncpy(dest, src, MAX_NAME_LEN - 1);
    dest[MAX_NAME_LEN - 1] = '\0';
}

// Remove comentários (começando com //)
void remove_comments(char* str) {
    char* comment = strstr(str, "//");
    if (comment) {
        *comment = '\0';
    }
}

// Converte string hexadecimal para uintptr_t
uintptr_t hex_to_uintptr(const char* hex_str) {
    return (uintptr_t)strtoull(hex_str, NULL, 0);
}

// Função principal do parser
Instruction* parse_instructions(FILE* file, int* count_insts, char** texts_out, int* text_size) {
    Instruction* instructions = NULL;
    *texts_out = NULL;
    *text_size = 0;
    *count_insts = 0;
    int capacity = 0;
    char line[256];

    while (fgets(line, sizeof(line), file)) {
        remove_comments(line);
        char* trimmed = trim(line);
        if (trimmed[0] == '\0') continue;

        // Processar a linha
        if (strncmp(trimmed, "Create(", 7) == 0) {
            char* start = trimmed + 7;
            char* end = strchr(start, ')');
            if (!end) continue;
            *end = '\0';
            char* name = trim(start);

            if (*count_insts >= capacity) {
                capacity = capacity ? capacity * 2 : 16;
                instructions = realloc(instructions, capacity * sizeof(Instruction));
            }
            instructions[*count_insts] = (Instruction){
                .type = INST_CREATE
            };
            safe_strcpy(instructions[*count_insts].args.create.process_name, name);
            (*count_insts)++;
        }
        else if (strcmp(trimmed, "Terminate();") == 0) {
            if (*count_insts >= capacity) {
                capacity = capacity ? capacity * 2 : 16;
                instructions = realloc(instructions, capacity * sizeof(Instruction));
            }
            instructions[*count_insts] = (Instruction){.type = INST_TERMINATE};
            (*count_insts)++;
        }
        else if (strstr(trimmed, "= mmap(")) {
            char* var_end = strstr(trimmed, "=");
            *var_end = '\0';
            char* var_name = trim(trimmed);

            char* start = strstr(var_end + 1, "(");
            if (!start) continue;
            start++;
            char* end = strchr(start, ')');
            if (!end) continue;
            *end = '\0';

            char* add_like = strtok(start, ",");
            char* size_str = strtok(NULL, ",");
            if (!add_like || !size_str) continue;

            int size = atoi(trim(size_str));
            add_like = trim(add_like);

            if (*count_insts >= capacity) {
                capacity = capacity ? capacity * 2 : 16;
                instructions = realloc(instructions, capacity * sizeof(Instruction));
            }
            instructions[*count_insts] = (Instruction){
                .type = INST_MMAP,
                .args.mmap.size = size,
                .args.mmap.add_like = hex_to_uintptr(add_like)
            };
            safe_strcpy(instructions[*count_insts].args.mmap.var_name, var_name);
            (*count_insts)++;
        }
        else if (strncmp(trimmed, "print_", 6) == 0) {
            char type = trimmed[6];
            char* start = strchr(trimmed, '(');
            if (!start) continue;
            start++;
            char* end = strchr(start, ')');
            if (!end) continue;
            *end = '\0';
            char* var_name = trim(start);

            InstType inst_type;
            if (type == 'n') inst_type = INST_PRINT_N;
            else if (type == 'p') inst_type = INST_PRINT_P;
            else if (type == 's') inst_type = INST_PRINT_S;
            else continue;

            if (*count_insts >= capacity) {
                capacity = capacity ? capacity * 2 : 16;
                instructions = realloc(instructions, capacity * sizeof(Instruction));
            }
            instructions[*count_insts] = (Instruction){
                .type = inst_type
            };
            safe_strcpy(instructions[*count_insts].args.print.var_name, var_name);
            (*count_insts)++;
        }
        else if (strstr(trimmed, "=") && !strstr(trimmed, "jump_eq")) {
            char* eq_pos = strchr(trimmed, '=');
            if (!eq_pos) continue;
            *eq_pos = '\0';
            char* var1 = trim(trimmed);
            char* expr = trim(eq_pos + 1);

            // Remover ponto e vírgula no final se existir
            if (expr[strlen(expr)-1] == ';') {
                expr[strlen(expr)-1] = '\0';
            }

            // Verificar se é operação aritmética
            char* op_pos = strpbrk(expr, "+-");
            if (op_pos) {
                char op = *op_pos;
                *op_pos = '\0';
                char* var2 = trim(expr);
                char* operand_str = trim(op_pos + 1);

                // Verificar se operando é número ou variável
                bool is_num = true;
                for (char* p = operand_str; *p; p++) {
                    if (!isdigit((unsigned char)*p) && *p != '-' && *p != '+') {
                        is_num = false;
                        break;
                    }
                }

                if (*count_insts >= capacity) {
                    capacity = capacity ? capacity * 2 : 16;
                    instructions = realloc(instructions, capacity * sizeof(Instruction));
                }

                if (is_num) {
                    int num = atoi(operand_str);
                    if (op == '+') {
                        instructions[*count_insts] = (Instruction){
                            .type = INST_ASSIGN_ADD_NUM,
                            .args.assign_num.num = num
                        };
                    } else {
                        instructions[*count_insts] = (Instruction){
                            .type = INST_ASSIGN_SUB_NUM,
                            .args.assign_num.num = num
                        };
                    }
                    safe_strcpy(instructions[*count_insts].args.assign_num.var1, var1);
                    safe_strcpy(instructions[*count_insts].args.assign_num.var2, var2);
                    (*count_insts)++;
                } else {
                    if (op == '+') {
                        instructions[*count_insts] = (Instruction){
                            .type = INST_ASSIGN_ADD_VAR
                        };
                    } else {
                        instructions[*count_insts] = (Instruction){
                            .type = INST_ASSIGN_SUB_VAR
                        };
                    }
                    safe_strcpy(instructions[*count_insts].args.assign_var.var1, var1);
                    safe_strcpy(instructions[*count_insts].args.assign_var.var2, var2);
                    safe_strcpy(instructions[*count_insts].args.assign_var.var3, operand_str);
                    (*count_insts)++;
                }
            } else {
                // Atribuição simples
                if (*count_insts >= capacity) {
                    capacity = capacity ? capacity * 2 : 16;
                    instructions = realloc(instructions, capacity * sizeof(Instruction));
                }

                bool is_num = true;
                for (char* p = expr; *p; p++) {
                    if (!isdigit((unsigned char)*p) && *p != '-' && *p != '+') {
                        is_num = false;
                        break;
                    }
                }

                if (is_num) {
                    instructions[*count_insts] = (Instruction){
                        .type = INST_ASSIGN_VAR_NUM,
                        .args.assign_var_num.num = atoi(expr)
                    };
                    safe_strcpy(instructions[*count_insts].args.assign_var_num.var1, var1);
                    (*count_insts)++;
                }
                else {
                    // var = 'c';
                    if (expr[0] == '\'' && expr[strlen(expr)-1] == '\'') {
                        expr[strlen(expr)-1] = '\0';
                        expr++;

                        instructions[*count_insts] = (Instruction) {
                            .type = INST_ASSIGN_VAR_NUM,
                            .args.assign_var_num.num = (int)expr[0]  // Valor ASCII do caractere
                        };
                        safe_strcpy(instructions[*count_insts].args.assign_var_num.var1, var1);
                        (*count_insts)++;  // IMPORTANTE: incrementar o contador
                    }
                    // var = "string";
                    else if (expr[0] == '"' && expr[strlen(expr)-1] == '"') {
                        expr[strlen(expr)-1] = '\0';
                        expr++;

                        int str_len = strlen(expr);
                        if (*texts_out) {
                            *texts_out = realloc(*texts_out, (*text_size) + str_len + 1);
                        } else {
                            *texts_out = malloc(str_len + 1);
                        }

                        // Copiar string para o buffer de textos
                        strcpy(*texts_out + *text_size, expr);

                        instructions[*count_insts] = (Instruction) {
                            .type = INST_ASSIGN_VAR_NUM,  // Armazena o offset como número
                            .args.assign_var_num.num = *text_size
                        };
                        safe_strcpy(instructions[*count_insts].args.assign_var_num.var1, var1);
                        (*count_insts)++;  // IMPORTANTE: incrementar o contador

                        *text_size += str_len + 1;  // +1 para o terminador nulo
                    }
                    // var1 = var2;
                    else {
                        instructions[*count_insts] = (Instruction) {
                            .type = INST_ASSIGN
                        };
                        safe_strcpy(instructions[*count_insts].args.assign.var1, var1);
                        safe_strcpy(instructions[*count_insts].args.assign.var2, expr);
                        (*count_insts)++;
                    }
                }
            }
        }
        else if (strncmp(trimmed, "label(", 6) == 0) {
            char* start = trimmed + 6;
            char* end = strchr(start, ')');
            if (!end) continue;
            *end = '\0';
            char* name = trim(start);
            add_label(name, *count_insts);
        }
        else if (strncmp(trimmed, "jump(", 5) == 0) {
            char* start = trimmed + 5;
            char* end = strchr(start, ')');
            if (!end) continue;
            *end = '\0';
            char* target = trim(start);

            if (*count_insts >= capacity) {
                capacity = capacity ? capacity * 2 : 16;
                instructions = realloc(instructions, capacity * sizeof(Instruction));
            }
            instructions[*count_insts] = (Instruction){
                .type = INST_JUMP,
                .args.jump.index = -1
            };
            safe_strcpy(instructions[*count_insts].args.jump.label, target);
            (*count_insts)++;
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
                if (!isdigit((unsigned char)*p) && *p != '-' && *p != '+') {
                    is_num = false;
                    break;
                }
            }

            if (*count_insts >= capacity) {
                capacity = capacity ? capacity * 2 : 16;
                instructions = realloc(instructions, capacity * sizeof(Instruction));
            }

            if (is_num) {
                int num = atoi(operand);
                instructions[*count_insts] = (Instruction){
                    .type = INST_JUMP_EQ_VAR_NUM,
                    .args.jump_eq_varnum = {
                        .target = {.index = -1},
                        .num = num
                    }
                };
                safe_strcpy(instructions[*count_insts].args.jump_eq_varnum.target.label, target);
                safe_strcpy(instructions[*count_insts].args.jump_eq_varnum.var, var);
            } else {
                instructions[*count_insts] = (Instruction){
                    .type = INST_JUMP_EQ_VAR_VAR,
                    .args.jump_eq_varvar = {
                        .target = {.index = -1}
                    }
                };
                safe_strcpy(instructions[*count_insts].args.jump_eq_varvar.target.label, target);
                safe_strcpy(instructions[*count_insts].args.jump_eq_varvar.var1, var);
                safe_strcpy(instructions[*count_insts].args.jump_eq_varvar.var2, operand);
            }
            (*count_insts)++;
        }
        else if (strncmp(trimmed, "jump_neq(", 9) == 0) {
            char* start = trimmed + 9;
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
                if (!isdigit((unsigned char)*p) && *p != '-' && *p != '+') {
                    is_num = false;
                    break;
                }
            }

            if (*count_insts >= capacity) {
                capacity = capacity ? capacity * 2 : 16;
                instructions = realloc(instructions, capacity * sizeof(Instruction));
            }

            if (is_num) {
                int num = atoi(operand);
                instructions[*count_insts] = (Instruction){
                    .type = INST_JUMP_N_EQ_VAR_NUM,
                    .args.jump_neq_varnum = {
                        .target = {.index = -1},
                        .num = num
                    }
                };
                safe_strcpy(instructions[*count_insts].args.jump_neq_varnum.target.label, target);
                safe_strcpy(instructions[*count_insts].args.jump_neq_varnum.var, var);
            } else {
                instructions[*count_insts] = (Instruction){
                    .type = INST_JUMP_N_EQ_VAR_VAR,
                    .args.jump_neq_varvar = {
                        .target = {.index = -1}
                    }
                };
                safe_strcpy(instructions[*count_insts].args.jump_neq_varvar.target.label, target);
                safe_strcpy(instructions[*count_insts].args.jump_neq_varvar.var1, var);
                safe_strcpy(instructions[*count_insts].args.jump_neq_varvar.var2, operand);
            }
            (*count_insts)++;
        }
        else if (strncmp(trimmed, "jump_lt(", 8) == 0) {
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
                if (!isdigit((unsigned char)*p) && *p != '-' && *p != '+') {
                    is_num = false;
                    break;
                }
            }

            if (*count_insts >= capacity) {
                capacity = capacity ? capacity * 2 : 16;
                instructions = realloc(instructions, capacity * sizeof(Instruction));
            }

            if (is_num) {
                int num = atoi(operand);
                instructions[*count_insts] = (Instruction){
                    .type = INST_JUMP_LT_VAR_NUM,
                    .args.jump_lt_varnum = {
                        .target = {.index = -1},
                        .num = num
                    }
                };
                safe_strcpy(instructions[*count_insts].args.jump_lt_varnum.target.label, target);
                safe_strcpy(instructions[*count_insts].args.jump_lt_varnum.var, var);
            } else {
                instructions[*count_insts] = (Instruction){
                    .type = INST_JUMP_LT_VAR_VAR,
                    .args.jump_lt_varvar = {
                        .target = {.index = -1}
                    }
                };
                safe_strcpy(instructions[*count_insts].args.jump_lt_varvar.target.label, target);
                safe_strcpy(instructions[*count_insts].args.jump_lt_varvar.var1, var);
                safe_strcpy(instructions[*count_insts].args.jump_lt_varvar.var2, operand);
            }
            (*count_insts)++;
        }
        else if (strncmp(trimmed, "jump_gt(", 8) == 0) {
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
                if (!isdigit((unsigned char)*p) && *p != '-' && *p != '+') {
                    is_num = false;
                    break;
                }
            }

            if (*count_insts >= capacity) {
                capacity = capacity ? capacity * 2 : 16;
                instructions = realloc(instructions, capacity * sizeof(Instruction));
            }

            if (is_num) {
                int num = atoi(operand);
                instructions[*count_insts] = (Instruction){
                    .type = INST_JUMP_GT_VAR_NUM,
                    .args.jump_gt_varnum = {
                        .target = {.index = -1},
                        .num = num
                    }
                };
                safe_strcpy(instructions[*count_insts].args.jump_gt_varnum.target.label, target);
                safe_strcpy(instructions[*count_insts].args.jump_gt_varnum.var, var);
            } else {
                instructions[*count_insts] = (Instruction){
                    .type = INST_JUMP_GT_VAR_VAR,
                    .args.jump_gt_varvar = {
                        .target = {.index = -1}
                    }
                };
                safe_strcpy(instructions[*count_insts].args.jump_gt_varvar.target.label, target);
                safe_strcpy(instructions[*count_insts].args.jump_gt_varvar.var1, var);
                safe_strcpy(instructions[*count_insts].args.jump_gt_varvar.var2, operand);
            }
            (*count_insts)++;
        }
        else if (strncmp(trimmed, "input_n(", 8) == 0) {
            char* start = trimmed + 8;
            char* end = strchr(start, ')');
            if (!end) continue;
            *end = '\0';
            char* var_name = trim(start);

            if (*count_insts >= capacity) {
                capacity = capacity ? capacity * 2 : 16;
                instructions = realloc(instructions, capacity * sizeof(Instruction));
            }
            instructions[*count_insts] = (Instruction){
                .type = INST_INPUT_N
            };
            safe_strcpy(instructions[*count_insts].args.input_n.var_name, var_name);
            (*count_insts)++;
        }
        else if (strncmp(trimmed, "input_s(", 8) == 0) {
            char* start = trimmed + 8;
            char* end = strchr(start, ')');
            if (!end) continue;
            *end = '\0';

            char* var_name = strtok(start, ",");
            char* size_str = strtok(NULL, ",");
            if (!var_name || !size_str) continue;

            int size = atoi(trim(size_str));
            var_name = trim(var_name);

            if (*count_insts >= capacity) {
                capacity = capacity ? capacity * 2 : 16;
                instructions = realloc(instructions, capacity * sizeof(Instruction));
            }
            instructions[*count_insts] = (Instruction){
                .type = INST_INPUT_S,
                .args.input_s.size = size
            };
            safe_strcpy(instructions[*count_insts].args.input_s.var_name, var_name);
            (*count_insts)++;
        }
        else {
            fprintf(stderr, "Instrução desconhecida: %s\n", trimmed);
        }

    }

    // Resolver labels
    for (int i = 0; i < *count_insts; i++) {
        Instruction* inst = &instructions[i];
        switch (inst->type) {
            case INST_JUMP: {
                int idx = find_label(inst->args.jump.label);
                if (idx != -1) {
                    inst->args.jump.index = idx;
                }
                break;
            }

            case INST_JUMP_EQ_VAR_NUM: {
                int idx = find_label(inst->args.jump_eq_varnum.target.label);
                if (idx != -1) {
                    inst->args.jump_eq_varnum.target.index = idx;
                }
                break;
            }

            case INST_JUMP_EQ_VAR_VAR: {
                int idx = find_label(inst->args.jump_eq_varvar.target.label);
                if (idx != -1) {
                    inst->args.jump_eq_varvar.target.index = idx;
                }
                break;
            }

            case INST_JUMP_N_EQ_VAR_NUM: {
                int idx = find_label(inst->args.jump_neq_varnum.target.label);
                if (idx != -1) {
                    inst->args.jump_neq_varnum.target.index = idx;
                }
                break;
            }

            case INST_JUMP_N_EQ_VAR_VAR: {
                int idx = find_label(inst->args.jump_neq_varvar.target.label);
                if (idx != -1) {
                    inst->args.jump_neq_varvar.target.index = idx;
                }
                break;
            }

            case INST_JUMP_LT_VAR_NUM: {
                int idx = find_label(inst->args.jump_lt_varnum.target.label);
                if (idx != -1) {
                    inst->args.jump_lt_varnum.target.index = idx;
                }
                break;
            }

            case INST_JUMP_LT_VAR_VAR: {
                int idx = find_label(inst->args.jump_lt_varvar.target.label);
                if (idx != -1) {
                    inst->args.jump_lt_varvar.target.index = idx;
                }
                break;
            }

            case INST_JUMP_GT_VAR_NUM: {
                int idx = find_label(inst->args.jump_gt_varnum.target.label);
                if (idx != -1) {
                    inst->args.jump_gt_varnum.target.index = idx;
                }
                break;
            }

            case INST_JUMP_GT_VAR_VAR: {
                int idx = find_label(inst->args.jump_gt_varvar.target.label);
                if (idx != -1) {
                    inst->args.jump_gt_varvar.target.index = idx;
                }
                break;
            }

            default:
                break;
        }
    }

    return instructions;
}

// Função para liberar memória
void free_instructions(Instruction* insts, int count) {
    free(insts);
    free_label_table();
}

/// function to get the instructions from the compiled binary.
void get_instructions(char* file_name, Instruction **instructions, size_t *count, char **texts_out, int *text_size) {
    const int fd = open(file_name, O_RDONLY);

    read(fd, text_size, sizeof(int));
    *texts_out = malloc(*text_size);
    read(fd, *texts_out, *text_size);

    size_t file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, sizeof(int) + *text_size, SEEK_SET);
    *count = (file_size - sizeof(int) - *text_size) / sizeof(Instruction);

    *instructions = malloc(*count * sizeof(Instruction));
    read(fd, *instructions, *count * sizeof(Instruction));
}

void print_instructions(const size_t count, Instruction *instructions) {
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
                printf("MMAP(%s, 0x%" PRIxPTR ", %d)\n",
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
            case INST_ASSIGN_VAR_NUM:
                printf("ASSIGN_VAR_NUM(%s = %d)\n",
                       instructions[i].args.assign_var_num.var1,
                       instructions[i].args.assign_var_num.num);
                break;
            case INST_ASSIGN_ADD_NUM:
                printf("ADD_NUM(%s = %s + %d)\n",
                       instructions[i].args.assign_num.var1,
                       instructions[i].args.assign_num.var2,
                       instructions[i].args.assign_num.num);
                break;
            case INST_ASSIGN_SUB_NUM:
                printf("SUB_NUM(%s = %s - %d)\n",
                       instructions[i].args.assign_num.var1,
                       instructions[i].args.assign_num.var2,
                       instructions[i].args.assign_num.num);
                break;
            case INST_ASSIGN_ADD_VAR:
                printf("ADD_VAR(%s = %s + %s)\n",
                       instructions[i].args.assign_var.var1,
                       instructions[i].args.assign_var.var2,
                       instructions[i].args.assign_var.var3);
                break;
            case INST_ASSIGN_SUB_VAR:
                printf("SUB_VAR(%s = %s - %s)\n",
                       instructions[i].args.assign_var.var1,
                       instructions[i].args.assign_var.var2,
                       instructions[i].args.assign_var.var3);
                break;
            case INST_JUMP:
                printf("JUMP(%d)\n", instructions[i].args.jump.index);
                break;
            case INST_JUMP_EQ_VAR_NUM:
                printf("JUMP_EQ_VAR_NUM(%d, %s, %d)\n",
                       instructions[i].args.jump_eq_varnum.target.index,
                       instructions[i].args.jump_eq_varnum.var,
                       instructions[i].args.jump_eq_varnum.num);
                break;
            case INST_JUMP_EQ_VAR_VAR:
                printf("JUMP_EQ_VAR_VAR(%d, %s, %s)\n",
                       instructions[i].args.jump_eq_varvar.target.index,
                       instructions[i].args.jump_eq_varvar.var1,
                       instructions[i].args.jump_eq_varvar.var2);
                break;
            case INST_JUMP_N_EQ_VAR_NUM:
                printf("JUMP_N_EQ_VAR_NUM(%d, %s, %d)\n",
                       instructions[i].args.jump_neq_varnum.target.index,
                       instructions[i].args.jump_neq_varnum.var,
                       instructions[i].args.jump_neq_varnum.num);
                break;
            case INST_JUMP_N_EQ_VAR_VAR:
                printf("JUMP_N_EQ_VAR_VAR(%d, %s, %s)\n",
                       instructions[i].args.jump_neq_varvar.target.index,
                       instructions[i].args.jump_neq_varvar.var1,
                       instructions[i].args.jump_neq_varvar.var2);
                break;
            case INST_JUMP_LT_VAR_NUM:
                printf("JUMP_LT_VAR_NUM(%d, %s, %d)\n",
                       instructions[i].args.jump_lt_varnum.target.index,
                       instructions[i].args.jump_lt_varnum.var,
                       instructions[i].args.jump_lt_varnum.num);
                break;
            case INST_JUMP_LT_VAR_VAR:
                printf("JUMP_LT_VAR_VAR(%d, %s, %s)\n",
                       instructions[i].args.jump_lt_varvar.target.index,
                       instructions[i].args.jump_lt_varvar.var1,
                       instructions[i].args.jump_lt_varvar.var2);
                break;
            case INST_JUMP_GT_VAR_NUM:
                printf("JUMP_GT_VAR_NUM(%d, %s, %d)\n",
                        instructions[i].args.jump_gt_varnum.target.index,
                        instructions[i].args.jump_gt_varnum.var,
                        instructions[i].args.jump_gt_varnum.num);
                break;
            case INST_JUMP_GT_VAR_VAR:
                printf("JUMP_GT_VAR_VAR(%d, %s, %s)\n",
                       instructions[i].args.jump_gt_varvar.target.index,
                       instructions[i].args.jump_gt_varvar.var1,
                       instructions[i].args.jump_gt_varvar.var2);
                break;
            case INST_INPUT_N:
                printf("INPUT_N(%s)\n", instructions[i].args.input_n.var_name);
                break;
            case INST_INPUT_S:
                printf("INPUT_S(%s, %d)\n",
                       instructions[i].args.input_s.var_name,
                       instructions[i].args.input_s.size);
                break;
            default:
                printf("Instrução desconhecida\n");
                break;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <arquivo_de_entrada> [<arquivo_de_saida>]\n", argv[0]);
        return 1;
    }

    FILE* file = fopen(argv[1], "r");
    if (!file) {
        perror("Erro ao abrir arquivo");
        return 1;
    }

    int count;
    char* texts_out;
    int text_size = 0;
    Instruction* instructions = parse_instructions(file, &count, &texts_out, &text_size);
    fclose(file);

    print_instructions(count, instructions);

    char* output_file_name = argc > 2 ? argv[2] : "output.bin";
    int output_file = open(output_file_name, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (output_file != -1) {
        write(output_file, &text_size, sizeof(int));
        write(output_file, texts_out, text_size);
        write(output_file, instructions, count * sizeof(Instruction));
    }
    else {
        perror("Erro ao abrir arquivo de saída");
    }
    free_instructions(instructions, count);
    close(output_file);


    printf("Tentando ler arquivo compilado\n");
    Instruction* read_instructions;
    size_t read_count;
    char* read_texts_out;
    int read_text_size;
    get_instructions(output_file_name, &read_instructions, &read_count, &read_texts_out, &read_text_size);

    printf("Texts:\n\t0: ");
    for (int i = 0; i < read_text_size; i++) {
        if (read_texts_out[i] == '\0') {
            printf("\n\t%d: ", i + 1);
        } else if (read_texts_out[i] == '\n') {
            printf("\\n");
        } else if (read_texts_out[i] == '\t') {
            printf("\\t");
        } else {
            printf("%c", read_texts_out[i]);
        }
    }
    printf("\nInstruções lidas:\n");
    print_instructions(read_count, read_instructions);

    return 0;
}
