
#include <cgi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ls.h"
#include "main.h"
#include "misc.h"
#include "lsof.h"
#include "usage.h"

int main(int argc, char **argv) {
    // 初始化
    AUTO_CGI_INPUT input = CGI_Get_Input();
    if (input == NULL) {
        return 0;
    }

    // if not exists fn key or fn value is empty
    INPUT *tmp = NULL;
    if ((tmp = CGI_Find_Parameter(input, "fn")) == NULL || !tmp->val) {
        return 0;
    }

    // 宣告並初始化功能結構
    func_element_t func_list[] = {
        {"ls", ls},
        {"usage", usage},
        {"lsof", lsof},
    };

    // 功能路由
    int i = 0;
    for (i = 0; i < SIZEOF(func_list); ++i) {
        // 不是要找的
        if (strcmp(func_list[i].name, tmp->val) != 0) {
            continue;
        }

        // 找到並執行
        if (func_list[i].fn(input) == 0) {
            break;
        }
    }

    return 0;
}
