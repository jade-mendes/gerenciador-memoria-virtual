//
// Created by nathan on 30/05/2025.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "n_interpreter.h"
#include "web_server.h"
#include "Simulador.h"

void print_config(SimulationConfig config) {
    printf("\n=== Configuração da Simulação ===\n");
    printf("Tamanho da Página: %d\n", config.PAGE_SIZE);
    printf("Tamanho da Memória Principal: %d\n", config.MP_SIZE);
    printf("Tamanho da Memória Secundária: %d\n", config.MS_SIZE);
    printf("Linhas da TLB: %d\n", config.TLB_SIZE);
    printf("Bits de Endereço Lógico: %d\n", config.BITS_LOGICAL_ADDRESS);
    printf("Política de Substituição: %s\n", config.SUB_POLICY_TYPE == SUB_POLICY_LRU ? "LRU" : "CLOCK");
    printf("=================================\n");
}

void parse_json_config(const char* json, SimulationConfig* config) {
    char policy[10];

    sscanf(json,
        "{\"PAGE_SIZE\":%u,\"MP_SIZE\":%u,\"MS_SIZE\":%u,\"TLB_SIZE\":%u,\"BITS_LOGICAL_ADDRESS\":%u,\"SUB_POLICY_TYPE\":\"%5[^\"]\"}",
        &config->PAGE_SIZE,
        &config->MP_SIZE,
        &config->MS_SIZE,
        &config->TLB_SIZE,
        &config->BITS_LOGICAL_ADDRESS,
        policy
    );
    config->TIME_SLICE = 10; // Valor padrão, pode ser alterado conforme necessário

    if (strcmp(policy, "LRU") == 0) {
        config->SUB_POLICY_TYPE = SUB_POLICY_LRU;
    } else if (strcmp(policy, "CLOCK") == 0) {
        config->SUB_POLICY_TYPE = SUB_POLICY_CLOCK;
    }
}

void start_simulation_button(char* json, int client_socket) {
    SimulationConfig config;
    parse_json_config(json, &config);

    if (simulador) {
        printf("\nReiniciando simulação com nova configuração...\n");
        destroy_simulator(simulador);
    }
    simulador = create_simulator(config);
    print_config(config);

    create_process_by_name("boot", 0);

    send_json(client_socket, "{\"status\": \"Simulação iniciada\"}");
}

void web_next_cycle(char* json, int client_socket) {
    if (!simulador) {
        write(client_socket, "HTTP/1.1 400 Bad Request", 24);
        return;
    }

    proxima_acao(simulador);

    char* json_response = generate_simulator_json(simulador);
    send_json(client_socket, json_response);
    free(json_response);
}

void web_set_user_input(char* json, int client_socket) {
    if (!simulador) {
        write(client_socket, "HTTP/1.1 400 Bad Request", 24);
        return;
    }

    // Extrai a entrada do JSON
    sscanf(json, "{\"input\": \"%[^\"]\"}", process_input);

    printf("Entrada do usuário: %s\n", process_input);

    write(client_socket, "HTTP/1.1 200 OK", 15);
}


#include <setjmp.h>
#include <signal.h>
#include <stdint.h>

// Variáveis estáticas para o tratamento de sinais
sigjmp_buf segfault_env;
volatile sig_atomic_t segfault_occurred = 0;
volatile sig_atomic_t segfault_cont = 0;

// Manipulador personalizado para SIGSEGV
void handle_segfault(int sig, siginfo_t *info, void *ucontext) {
    if (segfault_occurred) {
        segfault_cont++;
        printf("\n[SIGSEGV] Tentativa de acesso inválido detectada! Contagem: %d\n", segfault_cont);
        segfault_occurred = 0;
        siglongjmp(segfault_env, 1);
    } else {
        // Se não for do nosso interesse, repassa para o comportamento padrão
        struct sigaction default_act;
        default_act.sa_handler = SIG_DFL;
        sigemptyset(&default_act.sa_mask);
        default_act.sa_flags = 0;
        sigaction(sig, &default_act, NULL);
        kill(getpid(), sig);
    }
}

void web_get_data_from_address(char* json, int client_socket) {
    if (!simulador) {
        write(client_socket, "HTTP/1.1 400 Bad Request", 24);
        return;
    }

    uintptr_t address = 0;
    sscanf(json, "{\"address\": %lu}", &address);


    struct sigaction old_act, new_act = {0};
    new_act.sa_sigaction = handle_segfault;
    sigemptyset(&new_act.sa_mask);
    new_act.sa_flags = SA_SIGINFO;


    if (sigaction(SIGSEGV, &new_act, &old_act) < 0) {
        perror("Erro ao configurar manipulador de sinal");
        return;
    }

    if (sigsetjmp(segfault_env, 1)) {
        // Bloco executado após SIGSEGV
        segfault_occurred = 0;
        sigaction(SIGSEGV, &old_act, NULL); // Restaura o manipulador antigo
        send_json(client_socket, "{\"error\": \"[SIGSEGV] Endereço não alocado!\"}");
        return;
    }

    segfault_occurred = 1;
    const uint8_t value = *(uint8_t*)address;
    segfault_occurred = 0;

    sigaction(SIGSEGV, &old_act, NULL);

    // Se chegou aqui, o acesso foi bem-sucedido
    char response[64];
    snprintf(response, sizeof(response), "{\"value\": %d}", value);
    send_json(client_socket, response);
}