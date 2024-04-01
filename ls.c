#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>  //statbuf 用
#include <linux/limits.h>

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
    if (dir == NULL) {
        return;
    }

    // 找底下檔案或目錄
    struct stat statbuf = {};
    struct dirent *filePtr = NULL;
    while ((filePtr = readdir(dir)) != NULL) {
        // 連結路徑
        char *full_path;
        if (asprintf(&full_path, "%s/%s", path, filePtr->d_name) < 0) {
            continue;
        }

        // stat() 成功返回 0 失敗返回 -1 錯誤資訊放在 errno
        if (stat(full_path, &statbuf) == -1) {
            free(full_path);
            continue;
        }

        free(full_path);
        printf(
            "%o\t"
            "%8ju bytes\t"
            "%s\n",
            statbuf.st_mode % 512,
            statbuf.st_size,
            filePtr->d_name);
    }

    printf("\n");
    closedir(dir);
    return;
}