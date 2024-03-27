#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *my_readlink(char *fd_path, char *target_path) {
    ssize_t len = readlink(fd_path, target_path, sizeof(target_path) - 1);
    if (len == -1)
        return NULL;

    target_path[len] = '\0';
    return target_path;
}

void lsof(const char *path) {
    printf("path = %s\n", path);

    DIR *proc_dir, *d_fd;
    struct dirent *proc, *entry;
    char proc_fd_path[64], full_path[256], target_path[256];
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
            snprintf(full_path, sizeof(full_path), "/proc/%s/fd/%s", proc->d_name, entry->d_name);
            if ((fdlink = my_readlink(full_path, target_path)) == NULL)
                continue;

            printf("Process ID: %s, fd: %s, ", proc->d_name, entry->d_name);
            printf("readlink: %s\n", fdlink);
        }
        closedir(d_fd);
    }

    closedir(proc_dir);
    return;
}