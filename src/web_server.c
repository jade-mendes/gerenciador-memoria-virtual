//
// Created by nathan non 06/10/2024.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "main_menu.h"
#include "simulador_page.h"


#define PORT 7789


#define ADD_POST_A(url, callback) if (strcmp((url), request_url) == 0) { \
    char* json = find_json(strtok_r_buf); \
    if (json == NULL) { \
        write(client_socket, "HTTP/1.1 400 Bad Request", 24); \
        return; \
    } \
    callback(json, client_socket); \
}

#define ADD_POST(url, callback) if (strcmp((url), request_url) == 0) { \
    char* json = find_json(strtok_r_buf); \
    if (json == NULL) { \
        write(client_socket, "HTTP/1.1 400 Bad Request", 24); \
        return; \
    } \
    callback(json); \
    write(client_socket, "HTTP/1.1 200 OK", 15); \
}


// /run/media/nathan/Acer/Users/miche/Videos/Series e Filmes
//char* base_file_path = "/run/media/nathan/Acer/Users/miche/Videos/Series e Filmes/%s";
char* base_file_path = "./files/%s";

static int server_socket = -1;
#include <signal.h>
void handle_sigint(int sig) {
    if (server_socket != -1) {
        close(server_socket);
    }
    printf("\nServidor encerrado.\n");
    exit(0);
}

void format_file_path(char *path) {
    // replace all %20 with space
    char *p = path;
    while ((p = strstr(p, "%20")) != NULL) {
        *p = ' ';
        memmove(p + 1, p + 3, strlen(p + 3) + 1);
    }
}

void send_file(const int client_socket, char *filename, const char *content_type) {
    char path[1024];
    format_file_path(filename);
    sprintf(path, base_file_path, filename);
    printf("%s", path);
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        // Arquivo não encontrado
        const char *error_message = "HTTP/1.1 404 Not Found\r\n"
                                    "Content-Type: text/plain\r\n\r\n"
                                    "404 - File Not Found\n";
        write(client_socket, error_message, strlen(error_message));
        return;
    }

    // Enviar cabeçalho HTTP
    char header[1024];
    sprintf(header, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", content_type);
    write(client_socket, header, strlen(header));

    // Enviar conteúdo do arquivo
    char buffer[1024];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        write(client_socket, buffer, n);
    }

    fclose(file);
}

void send_json(const int client_socket, const char *json) {
    const char *header = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n";
    write(client_socket, header, strlen(header));
    write(client_socket, json, strlen(json));
}

char* find_json(const char* json) {
    char* start = strchr(json, '{');
    if (start == NULL) return NULL;

    char* end = strrchr(json, '}');
    if (end == NULL) return NULL;

    end[1] = '\0';
    return start;
}

void handle_request(const int client_socket) {
    char buffer[4096];
    const int n = read(client_socket, buffer, sizeof(buffer) - 1);
    if (n <= 0) return;
    buffer[n] = '\0';

    #ifdef DEBUG_WEB3
        write(STDOUT_FILENO, "Requisição recebida:\n", 21);
        write(STDOUT_FILENO, buffer, n);
    #endif

    char* strtok_r_buf;
    // Checar se é GET ou POST
    if (strncmp(buffer, "GET", 3) == 0) {
        // Tratar requisição GET
        if (strstr(buffer, "GET / ") != NULL) {
            send_file(client_socket, "index.html", "text/html");
        }
        // css
        else if (strstr(buffer, ".css ") != NULL) {
            char *filename = strtok_r(buffer + 5, " ", &strtok_r_buf);
            send_file(client_socket, filename, "text/css");
        }
        // html
        else if (strstr(buffer, ".html ") != NULL) {
            char *filename = strtok_r(buffer + 5, " ", &strtok_r_buf);
            send_file(client_socket, filename, "text/html");
        }
        else {
            char *filename = strtok_r(buffer + 5, " ", &strtok_r_buf);
            send_file(client_socket, filename, "text/plain");
        }
    }
    else if (strncmp(buffer, "POST", 4) == 0) {
        const char* request_url = strtok_r(buffer + 5, " ", &strtok_r_buf);

        // Aqui adiciona as rotas POST
        ADD_POST("/start-simulation", start_simulation_button)
        ADD_POST_A("/next-cycle", next_cycle)
    }

    else {
        write(client_socket, "HTTP/1.1 405 Method Not Allowed", 31);
    }
}

void* thread_handle_request(void* arg) {
    int client_socket = (intptr_t) arg;
    handle_request(client_socket);
    close(client_socket);
    return NULL;
}


void start_web_server() {
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(7789); // 7789
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(server_socket, 10) == -1) {
        perror("listen");
        exit(1);
    }

    printf("\n-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n"
            "Servidor iniciado na porta %d...\n"
            "\e[0;34mhttp://localhost:%d/\e[0m\n"
           "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n", PORT, PORT
    );

    signal(SIGTERM, handle_sigint);
    signal(SIGINT, handle_sigint);

    while (1) {
        const intptr_t client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &addr_len);

        if (client_socket == -1) {
            perror("Cannot accept connection");
            continue;
        }

        pthread_t thread_id;

        if (pthread_create(&thread_id, NULL, thread_handle_request, (void*) client_socket) != 0) {
            perror("pthread_create");
            close(client_socket);
        } else {
            pthread_detach(thread_id);
        }
    }

}