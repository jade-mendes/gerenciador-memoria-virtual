#ifndef MENU
#define MENU

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct menu_data{
    int PAGE_SIZE;
    int MP_SIZE;
    int MS_SIZE;
    int TLB_SIZE;
    int BITS_LOGICAL_ADDRESS;
    char FILE_NAME[100];
    int SUB_POLICY_TYPE;
} MDT;

int menu(MDT *data);


#endif