#include <cgi.h>
#include <stdio.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>  //statbuf 用

#include "main.h"
#include "misc.h"

// 檢查路徑是否位於...底下
bool is_under(char *input, char *under) {
    AUTO_STR resolved_path = NULL;
    resolved_path = realpath(input, NULL);
    if (resolved_path == NULL) {
        return false;
    }
    // printf("This source is at %s\n", resolved_path);
    // printf("strncmp %d\n", strncmp(resolved_path, under, strlen(under)));
    if (strncmp(resolved_path, under, strlen(under)) != 0) {
        return false;
    }

    return true;
}

// 目錄列舉
int ls(INPUT *input) {
    // 指定輸出 text/plain
    printf("Content-Type:text/plain; charset=utf-8\n\n");

    // 檢查輸入指標
    if (input == NULL) {
        return 0;
    }

    // if not exists fn key or fn value is empty
    INPUT *tmp = NULL;
    if ((tmp = CGI_Find_Parameter(input, "file")) == NULL || !tmp->val) {
        return 0;
    }

    // 如果路徑不在 /share 就離開
    if (!is_under(tmp->val, "/share")) {
        printf("Not under /share or the path is illegal\n");
        return 0;
    }

    // 取得目錄(dirent)結構指標
    AUTO_DIR dir = opendir(tmp->val);
    if (dir == NULL) {
        return 0;
    }

    // 找底下檔案或目錄
    struct dirent *filePtr = NULL;
    while ((filePtr = readdir(dir)) != NULL) {
        // 連結路徑
        AUTO_STR full_path = NULL;
        if (asprintf(&full_path, "%s/%s", tmp->val, filePtr->d_name) < 0) {
            continue;
        }

        // stat() 成功返回 0 失敗返回 -1 錯誤資訊放在 errno
        struct stat statbuf = {};
        if (stat(full_path, &statbuf) == -1) {
            continue;
        }

        printf(
            "%o\t"
            "%8ju bytes\t"
            "%s\n",
            statbuf.st_mode % 512,
            statbuf.st_size,
            filePtr->d_name);
    }
    printf("\n");

    // 離開
    return 0;
}