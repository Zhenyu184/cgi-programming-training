#include <stdio.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>  //statbuf 用

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

// 檢查路徑是否位於...底下
bool is_path_under(const char *path, const char *limit) {
    size_t len = strlen(limit);
    size_t path_len = strlen(path);

    // 確保路徑長度大於等於 "/share/" 的長度
    if (path_len < len) {
        return false;
    }

    // 檢查路徑前綴是否是 "/share/"
    if (strncmp(path, limit, len) == 0) {
        return true;
    }

    return false;
}

// 目錄列舉
void ls(const char *path) {
    // 如果路徑不在 /share 底下
    if (!is_path_under(path, "/share")) {
        return;
    }

    // 取得目錄(dirent)結構指標
    DIR *dir = opendir(path);
    if (dir == NULL) return;

    // 找底下檔案或目錄
    struct stat statbuf = {};
    struct dirent *filePtr = NULL;
    while ((filePtr = readdir(dir)) != NULL) {
        char *full_path = malloc(strlen(path) + strlen(filePtr->d_name) + 2);
        if (full_path == NULL) {
            return;
        }

        // 連結路徑
        sprintf(full_path, "%s/%s", path, filePtr->d_name);

        // stat() 成功返回 0 失敗返回 -1 錯誤資訊放在 errno
        if (stat(full_path, &statbuf) == -1) {
            free(full_path);
            continue;
        }

        printf(
            "%03d\t"
            "%8llu bytes\t"
            "%s\n",
            get_perm(statbuf),
            (unsigned long long)statbuf.st_size,
            filePtr->d_name);
        free(full_path);
    }

    printf("\n");
    closedir(dir);
    return;
}