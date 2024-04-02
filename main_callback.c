#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ls.h"
#include "lsof.h"
#include "usage.h"

#define SIZEOF(A) (sizeof(A) / sizeof(A[0]))

typedef int (*callback)(args_new_t *);

typedef struct args_new {
    char *path;
    unsigned long *pid;
    unsigned int second;
} args_new_t;

int ls_func(args_new_t *args) {
    printf("This is ls_func\n");
    return 0;
}

int lsof_func(args_new_t *args) {
    printf("This is lsof_func\n");
    return 0;
}

struct xxx {
    const char *name;
    callback fn;
} func_list[] = {
    {"ls", ls_func},
    {"lsof", lsof_func},
};

// 如果該段落是目標則回傳 value，不是則是 NULL
char *parse_value(char *section, const char *target) {
    char *equalPtr = strchr(section, '=');

    // 如果段落沒有 "=" 提早 return
    if (equalPtr == NULL) {
        return NULL;
    }

    // 將"="的"值"改成結束字元
    *equalPtr = '\0';
    char *key = section;
    char *value = equalPtr + 1;

    // 比較 key 是否為要找的目標
    if (strcmp(key, target) == 0) {
        return value;
    }

    // 什麼都沒找到
    return NULL;
}

// 解析整個參數字串輸出目標 key 的 value
char *parse_parameter(char *str, const char *target) {
    // 複製輸入字串指標，避免副作用
    char *str_copy = strdup(str);

    char *ret = NULL;
    char *temp = NULL;
    const char *delimiter = "&";
    for (temp = strtok(str_copy, delimiter); temp != NULL; temp = strtok(NULL, delimiter)) {
        // 如果該段落是要找的目標提早 return
        if ((ret = parse_value(temp, target))) {
            free(str_copy);
            return ret;
        }
    }

    // 什麼都沒匹配到
    free(str_copy);
    return NULL;
}

int main(int argc, char **argv) {
    // 讀取參數到 para
    char *para = getenv("QUERY_STRING");
    if (para == NULL) {
        printf("parse QUERY_STRING error\n");
        return 0;
    }

    // 讀取目標 fn 到 fn
    char *fn = parse_parameter(para, "fn");
    if (fn == NULL) {
        printf("parse fn error\n");
        return 0;
    }

    // 如果 fn 為空，就退出
    if (strlen(fn) == 0) {
        printf("fn is empty\n");
        return 0;
    }

    // 功能路由
    int i = 0;
    for (i = 0; i < SIZEOF(func_list); ++i) {
        // 不是要找的
        if (strcmp(func_list[i].name, fn) != 0) {
            continue;
        }

        // return func_list[i].fn(argc, argv);
    }

    return 0;
}