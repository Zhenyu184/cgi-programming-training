#include <ctype.h>
#include <stdio.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "main.h"
#include "misc.h"

// 解析 link
char *my_readlink(const char *fd_path) {
    // 分配 buffer，buffer 不能用 AUTO_STR 因為外面還須使用
    char *buffer = NULL;
    size_t bufsize = 256;
    char *tmp = realloc(buffer, bufsize);
    if (tmp == NULL) {
        // 內存分配失敗，釋放已分配的內存並返回 NULL
        free(buffer);
        return NULL;
    }
    buffer = tmp;

    // 呼叫 readlink 函數
    ssize_t len = readlink(fd_path, buffer, bufsize - 1);

    // 若連結長度大於緩衝區大小，則倍增緩衝區大小
    if (len >= 0 && (size_t)len >= bufsize - 1) {
        bufsize *= 2;
    }

    if (len == -1) {
        // 出錯，釋放內存並返回 NULL
        free(buffer);
        return NULL;
    }

    buffer[len] = '\0';  // 添加結尾字符
    return buffer;
}

// 相對路徑轉絕對路徑
char *absolute_path(char *relative_path) {
    char ret[PATH_MAX] = {};
    char *ret_ptr = (char *)malloc(sizeof(ret));
    if (ret_ptr == NULL) {
        return NULL;
    }
    realpath(relative_path, ret_ptr);
    return ret_ptr;
}

// 篩出指定目錄
bool grep_str(char *str1, char *str2) {
    char *result = strstr(str1, str2);
    if (result != NULL) {
        return true;
    }

    return false;
}

// 解析 /proc/<pid>/stat，回傳 comm
char *parse_pid_stat_comm(const char *pid) {
    char filepath[PATH_MAX] = {};
    snprintf(filepath, sizeof(filepath), "/proc/%s/stat", pid);

    // 開啟 stat 文件，如果找不到返回 0
    AUTO_FILE file = fopen(filepath, "r");
    if (file == NULL) {
        return NULL;
    }

    // 讀取 stat 文件
    char buffer[PATH_MAX] = {};
    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        return NULL;
    }

    // 解析 comm
    char ret[PATH_MAX] = {};
    char *ret_ptr = (char *)malloc(sizeof(ret));
    if (sscanf(buffer, "%*d %31s", ret_ptr) == EOF) {
        return NULL;
    }

    // 關閉文件
    return ret_ptr;
}

int lsof(INPUT *input) {
    // 指定輸出 text/plain
    printf("Content-Type:text/plain; charset=utf-8\n\n");

    // 檢查輸入指標
    if (input == NULL) {
        return 1;
    }

    // if not exists fn key or fn value is empty
    INPUT *tmp = NULL;
    if ((tmp = CGI_Find_Parameter(input, "file")) == NULL || !tmp->val) {
        return 2;
    }

    // 開啟 /proc 資料夾
    AUTO_DIR proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        return 3;
    }

    // 印出項目
    printf("%4s\t%16s\t%s\t%s\n", "PID", "COMM", "FD", "NAME");

    // 歷遍 /proc
    struct dirent *proc;
    while ((proc = readdir(proc_dir)) != NULL) {
        // 排除目錄名稱不是數字
        if (!isdigit(proc->d_name[0])) {
            continue;
        }

        // 組完整路徑
        char proc_fd_path[PATH_MAX] = {};
        snprintf(proc_fd_path, sizeof(proc_fd_path), "/proc/%s/fd/", proc->d_name);
        AUTO_DIR d_fd = opendir(proc_fd_path);
        if (d_fd == NULL) {
            continue;
        }

        // 歷遍 /proc/??/fd/
        struct dirent *entry = NULL;
        while ((entry = readdir(d_fd)) != NULL) {
            // 跳過 "." 與 ".."
            if (entry->d_name[0] == '.') {
                continue;
            }

            // 透過 name 找連結
            char full_name[CHAR_MAX] = {};
            snprintf(full_name, sizeof(full_name), "/proc/%s/fd/%s", proc->d_name, entry->d_name);
            AUTO_STR fdlink = NULL;
            if ((fdlink = my_readlink(full_name)) == NULL) {
                continue;
            }

            AUTO_STR real_path = absolute_path(tmp->val);
            if (real_path == NULL) {
                continue;
            }

            // 解析 comm
            char temp[CHAR_MAX] = {};
            AUTO_STR comm = parse_pid_stat_comm(proc->d_name);
            strcpy(temp, comm);

            // 篩選目標
            if (!grep_str(fdlink, real_path)) {
                continue;
            }

            printf("%4s\t%16s\t%s\t%s\n", proc->d_name, temp, entry->d_name, fdlink);
        }
    }

    return 0;
}