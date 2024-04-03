#include <stdio.h>
#include <stdlib.h>

#include <cgi.h>
#define _DEBUG 1
#define SAMPLE_MAIN 1
#define SAMPLE_NONE 0
#define SAMPLE_UPLOAD 2

int main() {
    printf("Content-Type: text;charset=utf-8\n\n");

    INPUT *tmp, *input;
    int g_number = 0;

    CGI_Init();
    input = (INPUT *)CGI_Get_Input();

    if ((tmp = CGI_Find_Parameter(input, "number")) != NULL) {
        g_number = atoi(tmp->val);
        printf("g_number = %d\n", g_number);
    }

    printf("HELLO CGI\n");

    return 0;
}