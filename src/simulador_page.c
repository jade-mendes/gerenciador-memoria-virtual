//
// Created by nathan on 31/05/2025.
//

#include <stdio.h>
#include <unistd.h>
#include "web_server.h"


void next_cycle(char* json, int client_socket) {
    // ler conteudo de ./exemplo.json e enviar.

    FILE* file = fopen("./files/exemplo.json", "r");
    // using send_json

    if (file == NULL) {
        write(client_socket, "HTTP/1.1 404 Not Found", 22);
        return;
    }

    char buffer[4096];
    size_t n = fread(buffer, 1, sizeof(buffer) - 1, file);
    fclose(file);
    buffer[n] = '\0';
    send_json(client_socket, buffer);
}
