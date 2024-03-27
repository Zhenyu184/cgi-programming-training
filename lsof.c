#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
        if (len >= 0 && (size_t)len >= bufsize - 1) {
            bufsize *= 2;
        }
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

void lsof(const char *path) {
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

            printf("Process ID: %s, fd: %s, ", proc->d_name, entry->d_name);
            printf("readlink: %s\n", fdlink);
        }
        closedir(d_fd);
    }

    closedir(proc_dir);
    return;
}