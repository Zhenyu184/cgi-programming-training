#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_PATH_LENGTH 1024

char *my_readlink(const char *fd_path) {
    // 開始分配一個較小的初始大小
    size_t bufsize = 256;
    char *buffer = NULL;
    ssize_t len;

    do {
        // 重新分配或分配內存
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
        if (len >= 0 && (size_t)len >= bufsize - 1)
            bufsize *= 2;

        break;
    } while (1);

    if (len == -1) {
        // 出錯，釋放內存並返回 NULL
        free(buffer);
        return NULL;
    }

    buffer[len] = '\0';  // 添加結尾字符
    return buffer;
}

char *make_absolute_path(const char *relative_path) {
    // 獲取當前工作目錄
    char cwd[MAX_PATH_LENGTH];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd() error");
        exit(EXIT_FAILURE);
    }

    // 創建一個新的字串來保存絕對路徑
    char *absolute_path = (char *)malloc(MAX_PATH_LENGTH * sizeof(char));
    if (absolute_path == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    char *token = strtok(strdup(relative_path), "/");  // 創建副本以避免更改原始字串
    strcpy(absolute_path, cwd);

    while (token != NULL) {
        if (strcmp(token, "..") == 0) {
            // 如果是".."，則上一級目錄
            char *last_slash = strrchr(absolute_path, '/');
            if (last_slash != NULL) {
                *last_slash = '\0';
            }
        } else if (strcmp(token, ".") != 0) {
            // 如果不是"."，則添加到絕對路徑中
            strcat(absolute_path, "/");
            strcat(absolute_path, token);
        }
        token = strtok(NULL, "/");
    }

    return absolute_path;
}

bool grep_str(char *str1, char *str2) {
    char *result = strstr(str1, str2);
    if (result != NULL)
        return true;

    return false;
}

void lsof(char *path) {
    printf("path = %s\n", path);

    DIR *proc_dir, *d_fd;
    struct dirent *proc, *entry;
    char proc_fd_path[64], full_name[256];
    char *fdlink;

    proc_dir = opendir("/proc");
    if (proc_dir == NULL)
        return;

    // 歷遍 /proc
    while ((proc = readdir(proc_dir)) != NULL) {
        // 排除目錄名稱不是數字
        if (!isdigit(proc->d_name[0]))
            continue;

        snprintf(proc_fd_path, sizeof(proc_fd_path), "/proc/%s/fd/", proc->d_name);
        d_fd = opendir(proc_fd_path);
        if (d_fd == NULL)
            continue;

        // 歷遍 /proc/??/fd/
        while ((entry = readdir(d_fd)) != NULL) {
            // 跳過 "." 與 ".."
            if (entry->d_name[0] == '.')
                continue;

            // 透過 name 找連結
            snprintf(full_name, sizeof(full_name), "/proc/%s/fd/%s", proc->d_name, entry->d_name);
            if ((fdlink = my_readlink(full_name)) == NULL)
                continue;

            // printf("readlink: %s\n", fdlink);
            // printf("%s\t%s\t%s\n", proc->d_name, entry->d_name, fdlink);

            // char *absolute_path = make_absolute_path(path);
            // if (absolute_path == NULL)
            //     continue;

            if (!grep_str(fdlink, path))
                continue;

            printf("%s\t%s\t%s\n", proc->d_name, entry->d_name, fdlink);
            // free(absolute_path);
        }
        closedir(d_fd);
    }

    closedir(proc_dir);
    return;
}