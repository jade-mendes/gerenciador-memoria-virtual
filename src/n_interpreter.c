// //
// // Created by Nathan on 03/06/2025.
// //
//
//
// #include <stddef.h>
// #include <stdint.h>
// #include <stdio.h>
//
// #include "n.h"
//
// void inst_create(const char *process_name, const size_t pid);
// void inst_terminate(const size_t pid);
// void inst_mmap(const char *var_name, uintptr_t add_like, int size, const size_t pid);
// void inst_print_n(const char *var_name, const size_t pid);
// void inst_print_p(const char *var_name, const size_t pid);
// void inst_print_s(const char *var_name, const size_t pid);
// void inst_assign(const char *var1, const char *var2, const size_t pid);
// void inst_assign_add_num(const char *var1, const char *var2, int num, const size_t pid);
// void inst_assign_sub_num(const char *var1, const char *var2, int num, const size_t pid);
// void inst_assign_add_var(const char *var1, const char *var2, const char *var3, const size_t pid);
// void inst_assign_sub_var(const char *var1, const char *var2, const char *var3, const size_t pid);
// void inst_label(const char *label_name, const size_t pid);
// void inst_jump(int target_index, const size_t pid);
// void inst_jump_eq_varnum(int target_index, const char *var, int num, const size_t pid);
// void inst_jump_eq_varvar(int target_index, const char *var1, const char *var2, const size_t pid);
// void inst_jump_neq_varnum(int target_index, const char *var, int num, const size_t pid);
// void inst_jump_neq_varvar(int target_index, const char *var1, const char *var2, const size_t pid);
// void inst_jump_lt_varnum(int target_index, const char *var, int num, const size_t pid);
// void inst_jump_lt_varvar(int target_index, const char *var1, const char *var2, const size_t pid);
// void inst_jump_gt_varnum(int target_index, const char *var, int num, const size_t pid);
// void inst_jump_gt_varvar(int target_index, const char *var1, const char *var2, const size_t pid);
// void inst_input_n(const char *var_name, const size_t pid);
// void inst_input_s(const char *var_name, int size, const size_t pid);
//
//
//
// void execute(const size_t index, const size_t pid, Instruction *instructions) {
//     Instruction *inst = &instructions[index];
//     switch (inst->type) {
//         case INST_CREATE:
//             inst_create(inst->args.create.process_name, pid);
//             break;
//         case INST_TERMINATE:
//             inst_terminate(pid);
//             break;
//         case INST_MMAP:
//             inst_mmap(inst->args.mmap.var_name, inst->args.mmap.add_like, inst->args.mmap.size, pid);
//             break;
//         case INST_PRINT_N:
//             inst_print_n(inst->args.print.var_name, pid);
//             break;
//         case INST_PRINT_P:
//             inst_print_p(inst->args.print.var_name, pid);
//             break;
//         case INST_PRINT_S:
//             inst_print_s(inst->args.print.var_name, pid);
//             break;
//         case INST_ASSIGN:
//             inst_assign(inst->args.assign.var1, inst->args.assign.var2, pid);
//             break;
//         case INST_ASSIGN_ADD_NUM:
//             inst_assign_add_num(inst->args.assign_num.var1, inst->args.assign_num.var2, inst->args.assign_num.num, pid);
//             break;
//         case INST_ASSIGN_SUB_NUM:
//             inst_assign_sub_num(inst->args.assign_num.var1, inst->args.assign_num.var2, inst->args.assign_num.num, pid);
//             break;
//         case INST_ASSIGN_ADD_VAR:
//             inst_assign_add_var(inst->args.assign_var.var1, inst->args.assign_var.var2, inst->args.assign_var.var3, pid);
//             break;
//         case INST_ASSIGN_SUB_VAR:
//             inst_assign_sub_var(inst->args.assign_var.var1, inst->args.assign_var.var2, inst->args.assign_var.var3, pid);
//             break;
//         case INST_LABEL:
//             inst_label(inst->args.label.label_name, pid);
//             break;
//         case INST_JUMP:
//             inst_jump(inst->args.jump.index, pid);
//             break;
//         case INST_JUMP_EQ_VAR_NUM:
//             inst_jump_eq_varnum(inst->args.jump_eq_varnum.target.index,
//                                inst->args.jump_eq_varnum.var,
//                                inst->args.jump_eq_varnum.num,
//                                pid);
//             break;
//         case INST_JUMP_EQ_VAR_VAR:
//             inst_jump_eq_varvar(inst->args.jump_eq_varvar.target.index,
//                               inst->args.jump_eq_varvar.var1,
//                               inst->args.jump_eq_varvar.var2,
//                               pid);
//             break;
//         case INST_JUMP_N_EQ_VAR_NUM:
//             inst_jump_neq_varnum(inst->args.jump_neq_varnum.target.index,
//                                 inst->args.jump_neq_varnum.var,
//                                 inst->args.jump_neq_varnum.num,
//                                 pid);
//             break;
//         case INST_JUMP_N_EQ_VAR_VAR:
//             inst_jump_neq_varvar(inst->args.jump_neq_varvar.target.index,
//                                inst->args.jump_neq_varvar.var1,
//                                inst->args.jump_neq_varvar.var2,
//                                pid);
//             break;
//         case INST_JUMP_LT_VAR_NUM:
//             inst_jump_lt_varnum(inst->args.jump_lt_varnum.target.index,
//                               inst->args.jump_lt_varnum.var,
//                               inst->args.jump_lt_varnum.num,
//                               pid);
//             break;
//         case INST_JUMP_LT_VAR_VAR:
//             inst_jump_lt_varvar(inst->args.jump_lt_varvar.target.index,
//                              inst->args.jump_lt_varvar.var1,
//                              inst->args.jump_lt_varvar.var2,
//                              pid);
//             break;
//         case INST_JUMP_GT_VAR_NUM:
//             inst_jump_gt_varnum(inst->args.jump_gt_varnum.target.index,
//                               inst->args.jump_gt_varnum.var,
//                               inst->args.jump_gt_varnum.num,
//                               pid);
//             break;
//         case INST_JUMP_GT_VAR_VAR:
//             inst_jump_gt_varvar(inst->args.jump_gt_varvar.target.index,
//                              inst->args.jump_gt_varvar.var1,
//                              inst->args.jump_gt_varvar.var2,
//                              pid);
//             break;
//
//         case INST_INPUT_N:
//             inst_input_n(inst->args.input_n.var_name, pid);
//             break;
//         case INST_INPUT_S:
//             inst_input_s(inst->args.input_s.var_name, inst->args.input_s.size, pid);
//             break;
//         default:
//             printf("Instrução desconhecida\n");
//             break;
//     }
// }
