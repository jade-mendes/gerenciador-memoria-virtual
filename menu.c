#include "menu.h"

int define_page_size(int *page_size);
int define_ms_size(int *ms_size);
int define_mp_size(int *mp_size);
int define_bits_logical_address(int *bits_logical_address);
int define_tlb_size(int *tlb_size);
int define_sub_policy_type(int *sub_policy_type);
int define_file(char *file);

int convertBinaryDimentions(char *text);

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
    char line[100];
    printf("Defina o tamanho da página:\n>> ");
    scanf("%s", line);
    convertBinaryDimentions(line);

}
int define_ms_size(int *ms_size){

}
int define_mp_size(int *mp_size){
    
}
int define_bits_logical_address(int *bits_logical_address){

}
int define_tlb_size(int *tlb_size){

}
int define_sub_policy_type(int *sub_policy_type){

}
int define_file(char *file){

}





// Dada uma string retorna o número de bits que ela representa
// Ex: 16B deveria retornar 128, 16b deveria retornar 16, 16 retornar -1, 16MB retorna 128000000
// Suportados: G, M, K, Gi, Mi, Ki, B e b

int convertBinaryDimentions(char *text){
    char *p = text, unity[2];
    int c = 0, n;
    while(*p >= '0' && *p <= '9'){
        c++; p++;
    }

    // Pega o número
    char *num = (char *) malloc(sizeof(char) * c);
    strncpy(num, text, c);
    n = atoi(num);
    free(num);

    // Pega a unidade
    sscanf(p, "%s", unity);

}
