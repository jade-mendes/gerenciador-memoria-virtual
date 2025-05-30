//
// Created by nathan on 30/05/2025.
//

#include <stdio.h>

typedef struct {
    int pageSize;
    int mpSize;
    int logicalBits;
    int tlbLines;
    char policy[16];
} SimulationConfig;

void print_config(SimulationConfig config) {
    printf("=== Configuração da Simulação ===\n");
    printf("Tamanho da Página: %d B\n", config.pageSize);
    printf("Tamanho da Memória Principal: %d B\n", config.mpSize);
    printf("Bits de Endereço Lógico: %d\n", config.logicalBits);
    printf("Linhas da TLB: %d\n", config.tlbLines);
    printf("Política de Substituição: %s\n", config.policy);
    printf("=================================\n");
}

void parse_json_config(const char* json, SimulationConfig* config) {
    // Extrai os valores do JSON
    sscanf(json,
        "{\"pageSize\":%d,\"mpSize\":%d,\"logicalBits\":%d,\"tlbLines\":%d,\"policy\":\"%15[^\"]\"}",
        &config->pageSize,
        &config->mpSize,
        &config->logicalBits,
        &config->tlbLines,
        config->policy
    );
}

void start_simulation_button(char* json) {
    SimulationConfig config;
    parse_json_config(json, &config);
    print_config(config);
}
