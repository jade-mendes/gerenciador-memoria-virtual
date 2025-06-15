//
// Created by Nathan on 03/06/2025.
//

#ifndef N_H
#define N_H

#define MAX_NAME_LEN 8 // 7 caracteres + \0
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Enumeração dos tipos de instrução
typedef enum {
    INST_CREATE_VAR,
    INST_CREATE_NUM,
    INST_TERMINATE,
    INST_MMAP,
    INST_PRINT_N,
    INST_PRINT_P,
    INST_PRINT_S,
    INST_ASSIGN,
    INST_ASSIGN_VAR_NUM,
    INST_ASSIGN_ADD_NUM,
    INST_ASSIGN_SUB_NUM,
    INST_ASSIGN_ADD_VAR,
    INST_ASSIGN_SUB_VAR,
    INST_LABEL,
    INST_JUMP,
    INST_JUMP_EQ_VAR_NUM,
    INST_JUMP_EQ_VAR_VAR,
    INST_JUMP_N_EQ_VAR_NUM,
    INST_JUMP_N_EQ_VAR_VAR,
    INST_JUMP_LT_VAR_NUM,
    INST_JUMP_LT_VAR_VAR,
    INST_JUMP_GT_VAR_NUM,
    INST_JUMP_GT_VAR_VAR,
    INST_INPUT_N,
    INST_INPUT_S,
} InstType;

// Estrutura para argumentos de saltos
typedef union  {
    int index;      // Índice numérico (se usado)
    char label[MAX_NAME_LEN];    // Nome da label
} JumpTarget;

// União para argumentos de instruções
typedef union {
    struct { char process_name[MAX_NAME_LEN]; } create_var;
    struct { int num; } create_num;
    struct { char var_name[MAX_NAME_LEN]; uintptr_t add_like; int size; } mmap;
    struct { char var_name[MAX_NAME_LEN]; } print;
    struct { char var1[MAX_NAME_LEN]; int num; } assign_var_num;
    struct { char var1[MAX_NAME_LEN]; char var2[MAX_NAME_LEN]; } assign;
    struct { char var1[MAX_NAME_LEN]; char var2[MAX_NAME_LEN]; int num; } assign_num;
    struct { char var1[MAX_NAME_LEN]; char var2[MAX_NAME_LEN]; char var3[MAX_NAME_LEN]; } assign_var;
    struct { char label_name[MAX_NAME_LEN]; } label;
    JumpTarget jump;
    struct { JumpTarget target; char var[MAX_NAME_LEN]; int num; } jump_eq_varnum;
    struct { JumpTarget target; char var1[MAX_NAME_LEN]; char var2[MAX_NAME_LEN]; } jump_eq_varvar;
    struct { JumpTarget target; char var[MAX_NAME_LEN]; int num; } jump_neq_varnum;
    struct { JumpTarget target; char var1[MAX_NAME_LEN]; char var2[MAX_NAME_LEN]; } jump_neq_varvar;
    struct { JumpTarget target; char var[MAX_NAME_LEN]; int num; } jump_lt_varnum;
    struct { JumpTarget target; char var1[MAX_NAME_LEN]; char var2[MAX_NAME_LEN]; } jump_lt_varvar;
    struct { JumpTarget target; char var[MAX_NAME_LEN]; int num; } jump_gt_varnum;
    struct { JumpTarget target; char var1[MAX_NAME_LEN]; char var2[MAX_NAME_LEN]; } jump_gt_varvar;
    struct { char var_name[MAX_NAME_LEN]; } input_n;
    struct { char var_name[MAX_NAME_LEN]; int size; } input_s;
} InstArgs;

// Estrutura de uma instrução
typedef struct {
    InstType type;
    InstArgs args;
} Instruction;

bool get_instructions(char* file_name, Instruction **instructions, size_t *count, char **texts_out, int *text_size);



#endif //N_H
