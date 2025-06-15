//
// Created by natha on 07/06/2025.
//

#include "n.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/// function to get the instructions from the compiled binary.
bool get_instructions(char* file_name, Instruction **instructions, size_t *count, char **texts_out, int *text_size) {
    const int fd = open(file_name, O_RDONLY);
    if (fd < 0) {
        perror("Erro ao abrir o arquivo de instruções");
        return false;
    }

    read(fd, text_size, sizeof(int));
    *texts_out = malloc(*text_size);
    read(fd, *texts_out, *text_size);

    size_t file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, sizeof(int) + *text_size, SEEK_SET);
    *count = (file_size - sizeof(int) - *text_size) / sizeof(Instruction);

    *instructions = malloc(*count * sizeof(Instruction));
    read(fd, *instructions, *count * sizeof(Instruction));
    close(fd);
    return true;
}