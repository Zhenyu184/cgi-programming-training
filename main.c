
#include <cgi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #include "ls.h"
// #include "lsof.h"
// #include "usage.h"

#define SIZEOF(A) (sizeof(A) / sizeof(A[0]))

typedef int (*callback)(int);
typedef struct func_element {
    const char *name;
    callback fn;
} func_element_t;

int ls_func(int args) {
    printf("This is ls_func\n");
    return 0;
}

int lsof_func(int args) {
    printf("This is lsof_func\n");
    return 0;
}

int main(int argc, char **argv) {
    // typedef struct tagINPUT {
    //     char *name;
    //     char *val;
    //     struct tagINPUT *next;
    // } INPUT;
    INPUT *tmp, *input;

    // 初始化
    CGI_Init();
    input = (INPUT *)CGI_Get_Input();

    // 搜尋參數 fn
    if ((tmp = CGI_Find_Parameter(input, "fn")) == NULL) {
        printf("failed to parse parameter \"fn\"\n");
        return 0;
    }

    // 如果 fn 為空，就退出
    char *fn = tmp->val;
    if (strlen(fn) == 0) {
        printf("fn is empty\n");
        return 0;
    }
    printf("function is \"%s\"\n", fn);

    // 功能路由
    func_element_t func_list[] = {
        {"ls", ls_func},
        {"lsof", lsof_func},
    };
    int i = 0;
    for (i = 0; i < SIZEOF(func_list); ++i) {
        // 不是要找的
        if (strcmp(func_list[i].name, fn) != 0) {
            continue;
        }

        // 找到並執行
        if (func_list[i].fn(100) == 0) {
            break;
        }
    }

    return 0;
}
