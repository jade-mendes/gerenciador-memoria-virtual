//
// Created by Nathan Pinheiro on 09/06/2025.
//

#ifndef STRINGBUILDER_H
#define STRINGBUILDER_H
#include <stddef.h>

typedef struct {
    char* data;
    size_t length;
    size_t capacity;
} StringBuilder;


void sb_init(StringBuilder* sb);
void sb_append(StringBuilder* sb, const char* str);
char* sb_finalize(StringBuilder* sb);


#endif //STRINGBUILDER_H
