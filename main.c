
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "ls.h"
#include "lsof.h"
#include "usage.h"

// 錯誤處理
void error_handling() {
}

// 如果該段落是目標則回傳 value，不是則是 NULL
char *parse_value(char *section, const char *target) {
    char *equalPtr = strchr(section, '=');

    // 如果段落沒有 "=" 提早 return
    if (equalPtr == NULL)
        return NULL;

    // 將"="的"值"改成結束字元
    *equalPtr = '\0';
    char *key = section;
    char *value = equalPtr + 1;

    // 比較 key 是否為要找的目標
    if (strcmp(key, target) == 0)
        return value;

    // 什麼都沒找到
    return NULL;
}

// 解析整個參數字串輸出目標 key 的 value
char *parse_parameter(char *str, const char *target) {
    // 複製輸入字串指標，避免副作用
    char *str_copy = strdup(str);

    const char *delimiter = "&";
    char *ret = NULL;
    char *temp = strtok(str_copy, delimiter);
    while (temp != NULL) {
        // 如果該段落是要找的目標提早 return
        if (ret = parse_value(temp, target))
            return ret;

        // 繼續找
        temp = strtok(NULL, delimiter);
    }

    // 什麼都沒匹配到
    return NULL;
}

// 檢查字串是否為正整數
bool is_integer(const char *num) {
    int i = 0;

    // 尋找每個字元
    for (i = 0; num[i]; ++i) {
        if (num[i] > '9' || num[i] < '0')
            return false;
    }

    // 都是數字但超過 10 位數(pid_t是int)
    if (i > 10)
        return false;
    return true;
}

int main(int argc, char *argv) {
    do {
        // 讀取參數到 para
        char *para = getenv("QUERY_STRING");
        if (para == NULL) {
            printf("parse QUERY_STRING error\n");
            break;
        }

        // 讀取目標 fn 到 fn
        char *fn = parse_parameter(para, "fn");
        if (fn == NULL) {
            printf("parse fn error\n");
            break;
        }

        // 如果 fn 為空，就退出
        if (strlen(fn) == 0) {
            printf("fn is empty\n");
            break;
        }

        // 功能路由
        if (strcmp(fn, "ls") == 0) {
            // 指定輸出 text
            printf("Content-Type:text; charset=utf-8\n\n");

            // 讀取目標 file 到 filepath
            char *filepath = parse_parameter(para, "file");
            if (filepath == NULL) {
                printf("parse file error\n");
                break;
            }

            // 如果 file 為空，就退出
            if (strlen(filepath) == 0) {
                printf("file is empty\n");
                break;
            }

            // 執行 ls
            ls(filepath);
            break;
        }

        if (strcmp(fn, "usage") == 0) {
            // 指定輸出
            printf("Content-Type:application/json; charset=utf-8\n\n");

            // 讀取目標 s 到 second
            char *second = parse_parameter(para, "s");
            if (second == NULL) {
                printf("parse second error\n");
                break;
            }

            // 讀取目標 pid 到 pid
            char *pid = parse_parameter(para, "pid");
            if (pid == NULL) {
                printf("parse pid error\n");
                break;
            }

            // 如果 pid 或 second 不是正整數
            if (!is_integer(pid) || !is_integer(second)) {
                printf("pid or second is not integer\n");
                break;
            }

            // 如果 pid 為空，就預設為自己
            if (strlen(pid) == 0)
                pid = "self";

            // 如果 second 為空，就假設 1
            if (strlen(second) == 0)
                second = "1";

            // 執行 usage
            usage(pid, second);
            break;
        }

        if (strcmp(fn, "lsof") == 0) {
            // 指定輸出 text
            printf("Content-Type:text; charset=utf-8\n\n");

            // 讀取目標 file 到 filepath
            char *filepath = parse_parameter(para, "file");
            if (filepath == NULL) {
                printf("parse file error\n");
                break;
            }

            // 如果 file 為空，就退出
            if (strlen(filepath) == 0) {
                printf("file is empty\n");
                break;
            }

            // 執行 lsof
            lsof(filepath);
            break;
        }

    } while (false);

    return 0;
}
