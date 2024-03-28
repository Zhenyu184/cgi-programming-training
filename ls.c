#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// 讀取權限
int get_perm(struct stat fileStat) {
    int ret = 0;
    ret += (fileStat.st_mode & S_IRUSR) ? 4 : 0;
    ret += (fileStat.st_mode & S_IWUSR) ? 2 : 0;
    ret += (fileStat.st_mode & S_IXUSR) ? 1 : 0;
    ret *= 10;
    ret += (fileStat.st_mode & S_IRGRP) ? 4 : 0;
    ret += (fileStat.st_mode & S_IWGRP) ? 2 : 0;
    ret += (fileStat.st_mode & S_IXGRP) ? 1 : 0;
    ret *= 10;
    ret += (fileStat.st_mode & S_IROTH) ? 4 : 0;
    ret += (fileStat.st_mode & S_IWOTH) ? 2 : 0;
    ret += (fileStat.st_mode & S_IXOTH) ? 1 : 0;
    return ret;
}

// 目錄列舉
void ls(const char *path) {
    struct dirent *filePtr = NULL;
    struct stat statbuf;
    DIR *dir = NULL;

    // 取得目錄(dirent)結構指標
    dir = opendir(path);
    if (dir == NULL) {
        error_handling();
        return;
    }

    // 找底下檔案或目錄
    while ((filePtr = readdir(dir)) != NULL) {
        char *full_path = malloc(strlen(path) + strlen(filePtr->d_name) + 2);
        if (full_path == NULL) {
            error_handling();
            return;
        }

        // 連結路徑
        sprintf(full_path, "%s/%s", path, filePtr->d_name);

        // stat() 成功返回 0 失敗返回 -1 錯誤資訊放在 errno
        if (stat(full_path, &statbuf) == -1) {
            error_handling();
            free(full_path);
            continue;
        }

        printf("%03d\t", get_perm(statbuf));
        printf("%8lld bytes\t", (unsigned long long)statbuf.st_size);
        printf("%s\n", filePtr->d_name);
        free(full_path);
    }

    printf("\n");
    closedir(dir);
    return;
}