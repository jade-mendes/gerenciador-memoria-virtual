#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "menu.h"

MDT *create_default_menu_data(){
    MDT *data = (MDT *) malloc(sizeof(MDT));
    data->PAGE_SIZE = 128;
    data->MP_SIZE = 512;
    data->MS_SIZE = 8096;
    data->TLB_SIZE = 16;
    data->BITS_LOGICAL_ADDRESS = 16;
    strcpy(data->FILE_NAME, "data.txt");
    data->SUB_POLICY_TYPE = 0; // 0 = LRU e 1 = Relógio
    return data;
}

int main(){
    MDT *data = create_default_menu_data();
    while(!menu(data));
    free(data);
    return 0;
}