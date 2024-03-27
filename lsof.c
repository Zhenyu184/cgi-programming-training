#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FAST_FUNC __attribute__((regparm(3), stdcall))

enum {
    PSSCAN_PID = 1 << 0,
    PSSCAN_EXE = 1 << 8,
};

typedef struct procps_status_t {
    DIR *dir;
    char *exe;
    unsigned pid;
    unsigned ppid;
    unsigned pgid;
    unsigned sid;
    unsigned uid;
    unsigned gid;
} procps_status_t;

void lsof(const char *path) {
    printf("path = %s\n", path);

    DIR *proc_dir;
    struct dirent *proc_entry;
    char proc_fd_path[64];

    proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        perror("Failed to open /proc directory");
        return;
    }

    // 歷遍 /proc
    while ((proc_entry = readdir(proc_dir)) != NULL) {
        // 檢查目錄名稱是數字
        if (!isdigit(proc_entry->d_name[0]))
            continue;

        // 組 fd 目錄路徑
        snprintf(proc_fd_path, sizeof(proc_fd_path), "/proc/%s/fd/", proc_entry->d_name);

        // 開 /proc/.../fd/ 底下所有 process 目錄
        DIR *d_fd = opendir(proc_fd_path);

        if (d_fd == NULL)
            continue;

        // 歷遍 /proc/.../fd/
        struct dirent *fd_entry;
        while ((fd_entry = readdir(d_fd)) != NULL) {
            // 忽略 "." 與 ".."
            if (fd_entry->d_name[0] == '.')
                continue;

            char fd_path[256];
            char target_path[256];
            snprintf(fd_path, sizeof(fd_path), "/proc/%s/fd/%s", proc_entry->d_name, fd_entry->d_name);
            ssize_t len = readlink(fd_path, target_path, sizeof(target_path) - 1);
            if (len != -1) {
                target_path[len] = '\0';
                printf("Process ID: %s, fd: %s, target: %s\n", proc_entry->d_name, fd_entry->d_name, target_path);
            } else {
                perror("readlink error");
            }
        }
        closedir(d_fd);
    }

    closedir(proc_dir);
    return;
}