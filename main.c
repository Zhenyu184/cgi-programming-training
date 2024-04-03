
#include <cgi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

#define SIZEOF(A) (sizeof(A) / sizeof(A[0]))

char *target_args_list[] = {"fn", "file", "pid", "s"};

int ls_func(int args) {
    printf("This is ls_func\n");
    return 0;
}

int usage_func(int args) {
    printf("This is usage_func\n");
    return 0;
}

int lsof_func(int args) {
    printf("This is lsof_func\n");
    return 0;
}

int main(int argc, char **argv) {
    // CGI 處理
    INPUT *tmp, *input;

    // 初始化
    CGI_Init();
    input = (INPUT *)CGI_Get_Input();

    // 宣告並初始化參數結構
    args_t args = {
        .fn = "",
        .file = "./",
        .pid = 0,
        .s = 1,
    };

    // 將參數一次 parse 完
    int i = 0;
    for (i = 0; i < SIZEOF(target_args_list); ++i) {
        // 搜尋參數所有目標參數
        if ((tmp = CGI_Find_Parameter(input, target_args_list[i])) == NULL) {
            printf("failed to parse parameter \"%s\"\n", target_args_list[i]);
            continue;
        }

        // 值為空
        if (strlen(tmp->val) == 0) {
            continue;
        }

        // 各部參數處理(目前還未想到如何用loop做)
        if (strncmp(tmp->name, "fn", 2) == 0) {
            args.fn = tmp->val;
        } else if (strncmp(tmp->name, "file", 4) == 0) {
            args.file = tmp->val;
        } else if (strncmp(tmp->name, "pid", 3) == 0) {
            args.pid = atoi(tmp->val);
        } else if (strncmp(tmp->name, "s", 1) == 0) {
            args.s = atoi(tmp->val);
        }
    }

    // 宣告並初始化功能結構
    func_element_t func_list[] = {
        {"ls", ls_func},
        {"usage", usage_func},
        {"lsof", lsof_func},
    };

    // 功能路由
    for (i = 0; i < SIZEOF(func_list); ++i) {
        // 不是要找的
        if (strcmp(func_list[i].name, args.fn) != 0) {
            continue;
        }

        // 找到並執行
        if (func_list[i].fn(100) == 0) {
            break;
        }
    }

    return 0;
}
