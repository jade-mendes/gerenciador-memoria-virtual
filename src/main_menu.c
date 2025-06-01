//
// Created by nathan on 30/05/2025.
//

#include <stdio.h>

typedef struct {
    int PAGE_SIZE;
    char PAGE_SIZE_DIM[4];
    int MP_SIZE;
    char MP_SIZE_DIM[4];
    int MS_SIZE;
    char MS_SIZE_DIM[4];
    int TLB_SIZE;
    int BITS_LOGICAL_ADDRESS;
    int SUB_POLICY_TYPE;
    char FILE_NAME[100];
} SimulationConfig;

void print_config(SimulationConfig config) {
    printf("=== Configuração da Simulação ===\n");
    printf("Tamanho da Página: %d%s\n", config.PAGE_SIZE, config.PAGE_SIZE_DIM);
    printf("Tamanho da Memória Principal: %d%s\n", config.MP_SIZE, config.MP_SIZE_DIM);
    printf("Tamanho da Memória Secundária: %d%s\n", config.MS_SIZE, config.MS_SIZE_DIM);
    printf("Linhas da TLB: %d\n", config.TLB_SIZE);
    printf("Bits de Endereço Lógico: %d\n", config.BITS_LOGICAL_ADDRESS);
    printf("Política de Substituição: %s\n", config.SUB_POLICY_TYPE);
    printf("Arquivo de leitura: %s\n", config.FILE_NAME);
    printf("=================================\n");
}

void parse_json_config(const char* json, SimulationConfig* config) {
    // Extrai os valores do JSON
    sscanf(json,
        "{\"PAGE_SIZE\":%d,\"PAGE_SIZE_DIM\":%s,\"MP_SIZE\":%d,\"MP_SIZE_DIM\":%s,\"MS_SIZE\":%d,\"MS_SIZE_DIM\":%s,\"BITS_LOGICAL_ADDRESS\":%d,\"TLB_SIZE\":%d,\"SUB_POLICY_TYPE\":%s,\"FILE_NAME\":%s\"}",
        &config->PAGE_SIZE,
        config->PAGE_SIZE_DIM,
        &config->MP_SIZE,
        config->MP_SIZE_DIM,
        &config->MS_SIZE,
        config->MS_SIZE_DIM,
        &config->TLB_SIZE,
        &config->BITS_LOGICAL_ADDRESS,
        &config->SUB_POLICY_TYPE,
        config->FILE_NAME
    );
}

void start_simulation_button(char* json) {
    SimulationConfig config;
    parse_json_config(json, &config);
    print_config(config);
}
