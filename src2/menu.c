#include "menu.h"

int define_page_size(int *page_size);
int define_ms_size(int *ms_size);
int define_mp_size(int *mp_size);
int define_bits_logical_address(int *bits_logical_address);
int define_tlb_size(int *tlb_size);
int define_sub_policy_type(int *sub_policy_type);
int define_file(char *file);

int convert_binary_dimentions(char *text);

int menu(MDT *data){
    char c;
    int op, flag = 1;

    printf("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
    printf("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-Menu=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
    printf("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
    printf("1) Iniciar simulação\n");
    printf("2) Definir tamanho da página (padrão 16B)\n");
    printf("3) Definir tamanho da MP (padrão 1024B)\n");
    printf("4) Definir tamanho da MS (262144B)\n");
    printf("5) Definir número de bits de endereço lógico (padrão 16b)\n");
    printf("6) Definir número de linhas da TLB (padrão 16)\n");
    printf("7) Definir política de substituição (padrão LRU)\n");
    printf("8) Definir arquivo de leitura (padrão data.txt)\n\n");
    printf("Qual opção você deseja?\n>> ");

    scanf("%d", &op);
    switch(op){
    case 1: return 1; break;
    case 2: flag = define_page_size(&data->PAGE_SIZE); break;
    case 3: flag = define_mp_size(&data->MP_SIZE); break;
    case 4: flag = define_ms_size(&data->MS_SIZE); break;
    case 5: flag = define_bits_logical_address(&data->BITS_LOGICAL_ADDRESS); break;
    case 6: flag = define_tlb_size(&data->TLB_SIZE); break;
    case 7: flag = define_sub_policy_type(&data->SUB_POLICY_TYPE); break;
    case 8: flag = define_file(data->FILE_NAME); break;
    }
    if(flag) printf("A opção digitada é inválida!\n");

    printf("\n-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n\n");
    return 0;
}

int define_page_size(int *page_size){

    // Lê a linha
    char line[100];
    printf("Defina o tamanho da página:\n>> ");
    scanf("%s", line);

    // Converte o valor para bits
    int n = convert_binary_dimentions(line);
    
    if(n != -1) *page_size = n;
    else return 1;
    return 0;
}
int define_ms_size(int *ms_size){

    // Lê a linha
    char line[100];
    printf("Defina o tamanho da MS:\n>> ");
    scanf("%s", line);

    // Converte o valor para bits
    int n = convert_binary_dimentions(line);
    
    if(n != -1) *ms_size = n;
    else return 1;
    return 0;
}
int define_mp_size(int *mp_size){

    // Lê a linha
    char line[100];
    printf("Defina o tamanho da MP:\n>> ");
    scanf("%s", line);

    // Converte o valor para bits
    int n = convert_binary_dimentions(line);
    
    if(n != -1) *mp_size = n;
    else return 1;
    return 0;
    
}
int define_bits_logical_address(int *bits_logical_address){
    int n;
    printf("Defina o número de bis para o endereçamento lógico da MS:\n>> ");
    scanf("%n", n);
    *bits_logical_address = n;
    return 0;
}
int define_tlb_size(int *tlb_size){
    int n;
    printf("Defina o número de entradas da TLB:\n>> ");
    scanf("%n", n);
    *tlb_size = n;
    return 0;
}
int define_sub_policy_type(int *sub_policy_type){
    int n;
    printf("Defina a política de substituição:\n1) LRU (padrão)\n2) Relógio (com bit u)\n>> ");
    scanf("%n", n);
    if((n = 1) || (n = 2)) *sub_policy_type = n;
    else return 1;
    return 0;
}
int define_file(char *file){
    
}

// Dada uma string retorna o número de bits que ela representa
// Ex: 16B deveria retornar 128, 16b deveria retornar 16, 16 retornar -1, 16MB retorna 128000000
// Suportados: G, M, K, Gi, Mi, Ki, B e b
int convert_binary_dimentions(char *text){

    // Acha o início da unidade
    char *p = text, unity[4];
    int c = 0, n;
    while(*p >= '0' && *p <= '9'){
        c++; p++;
    }

    // Pega o número
    char *num = (char *) malloc(sizeof(char) * c + 1);
    strncpy(num, text, c);
    num[c] = '\0';
    n = atoi(num);
    free(num);

    // Se não tiver unidade, retorna como se fosse bit
    if(*p == '\0') return n;

    // Pega a unidade
    sscanf(p, "%s", unity);
    int u;
    if(strcmp(unity, "b") == 0) u = 1;
    else if(strcmp(unity, "B") == 0) u = 8;
    else if(strcmp(unity, "Mb") == 0) u = 1000000;
    else if(strcmp(unity, "MB") == 0) u = 8000000;
    else if(strcmp(unity, "Mib") == 0) u = 1048576;
    else if(strcmp(unity, "MiB") == 0) u = 8388608;
    else{
        perror("Tamanho não reconhecido!\n"); 
        return -1;
    }

    // Retorna a quantidade em bits
    return u * n;
}
