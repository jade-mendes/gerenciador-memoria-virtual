//
// Created by Nathan Pinheiro on 17/06/2025.
//

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>

// Buffer para salvar o contexto de execução
sigjmp_buf recovery_point;

// Handler para SIGSEGV
void handle_sigsegv(int sig) {
    write(STDERR_FILENO, "\nSIGSEGV capturado! Recuperando...\n", 35);
    // Retorna ao ponto salvo com código de retorno 1
    siglongjmp(recovery_point, 1);
}

int main() {
    struct sigaction sa;

    // Configurar o handler de sinal
    sa.sa_handler = handle_sigsegv;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_NODEFER;  // Não bloquear o sinal durante o handler

    // Registrar handler para SIGSEGV
    if (sigaction(SIGSEGV, &sa, NULL)) {
        perror("sigaction");
        exit(1);
    }

    printf("Estado inicial: Pressione ENTER para causar uma falha de segmentação...");
    getchar();

    // Ponto de recuperação (salva contexto)
    if (sigsetjmp(recovery_point, 1)) {
        printf("Processo recuperado com sucesso!\n");
        printf("O processo NÃO foi terminado pelo SIGSEGV.\n");
        char c;
        do {
            printf("Digite 'q' para sair ou qualquer outra tecla para continuar: ");
            c = getchar();
            while (getchar() != '\n');  // Limpar o buffer de entrada
        } while (c != 'q');
        return 0;
    }

    // Causando uma violação de segmentação
    int *ptr = NULL;
    *ptr = 42;  // Escrita em endereço inválido

    // Esta linha nunca será executada
    printf("Operação concluída (isso não deve aparecer)\n");
    return 0;
}

