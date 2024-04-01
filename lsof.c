#include <ctype.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

char *my_readlink(const char *fd_path) {
    // 開始分配一個較小的初始大小
    ssize_t len = 0;
    char *buffer = NULL;
    size_t bufsize = 256;

    do {
        // 重新分配或分配內存
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
        len = readlink(fd_path, buffer, bufsize - 1);

        // 若連結長度大於緩衝區大小，則倍增緩衝區大小
        if (len >= 0 && (size_t)len >= bufsize - 1) {
            bufsize *= 2;
        }

    } while (1);

    if (len == -1) {
        // 出錯，釋放內存並返回 NULL
        free(buffer);
        return NULL;
    }

    buffer[len] = '\0';  // 添加結尾字符
    return buffer;
}

char *absolute_path(char *relative_path) {
    char *ret = (char *)malloc(1024 * sizeof(char));
    if (ret == NULL) {
        return NULL;
    }
    realpath(relative_path, ret);
    return ret;
}

bool grep_str(char *str1, char *str2) {
    char *result = strstr(str1, str2);
    if (result != NULL) {
        return true;
    }

    return false;
}

// 解析 /proc/<pid>/stat，回傳 comm
char *parse_pid_stat_comm(const char *pid) {
    char filepath[32] = {};
    snprintf(filepath, sizeof(filepath), "/proc/%s/stat", pid);

    // 開啟 stat 文件，如果找不到返回 0
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        return NULL;
    }

    // 讀取 stat 文件
    char buffer[2048] = {};
    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        return NULL;
    }

    // 解析 comm
    char *ret = (char *)malloc(32 * sizeof(char));
    if (sscanf(buffer, "%*d %31s", ret) == EOF) {
        fclose(file);
        return NULL;
    }

    // 關閉文件
    fclose(file);
    return ret;
}

void lsof(char *path) {
    // printf("path = %s\n", path);

    DIR *proc_dir,
        *d_fd;
    struct dirent *proc, *entry;
    char proc_fd_path[64], full_name[256];
    char *fdlink;

    proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        return;
    }

    printf("%4s\t%16s\t%s\t%s\n", "PID", "COMM", "FD", "NAME");

    // 歷遍 /proc
    while ((proc = readdir(proc_dir)) != NULL) {
        // 排除目錄名稱不是數字
        if (!isdigit(proc->d_name[0])) {
            continue;
        }

        snprintf(proc_fd_path, sizeof(proc_fd_path), "/proc/%s/fd/", proc->d_name);
        d_fd = opendir(proc_fd_path);
        if (d_fd == NULL) {
            continue;
        }

        // 歷遍 /proc/??/fd/
        while ((entry = readdir(d_fd)) != NULL) {
            // 跳過 "." 與 ".."
            if (entry->d_name[0] == '.') {
                continue;
            }

            // 透過 name 找連結
            snprintf(full_name, sizeof(full_name), "/proc/%s/fd/%s", proc->d_name, entry->d_name);
            if ((fdlink = my_readlink(full_name)) == NULL) {
                continue;
            }

            char *real_path = absolute_path(path);
            if (real_path == NULL) {
                continue;
            }

            // 解析 comm
            char temp[16] = {};
            strcpy(temp, parse_pid_stat_comm(proc->d_name));

            // 篩選目標
            if (!grep_str(fdlink, real_path)) {
                continue;
            }

            printf("%4s\t%16s\t%s\t%s\n", proc->d_name, temp, entry->d_name, fdlink);
            free(real_path);
        }
        closedir(d_fd);
    }

    closedir(proc_dir);
    return;
}