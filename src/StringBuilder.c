//
// Created by Nathan Pinheiro on 09/06/2025.
//


#include <stdlib.h>
#include <string.h>
#include "StringBuilder.h"



void sb_init(StringBuilder* sb) {
    sb->data = NULL;
    sb->length = 0;
    sb->capacity = 0;
}

void sb_append(StringBuilder* sb, const char* str) {
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

char* sb_finalize(StringBuilder* sb) {
    char* result = sb->data;
    sb_init(sb);
    return result;
}
