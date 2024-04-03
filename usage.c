#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <asm/param.h>

#include "main.h"  // 為了INPUT
#include "misc.h"

// 取得核心數
int get_cpu_cores() {
    int cores = sysconf(_SC_NPROCESSORS_ONLN);
    // sysconf 如果錯誤會返回 -1 ，並設定 errno
    // 為了避免除 0，令 cores = 0 返回 1
    if (cores <= 0)
        return 1;
    return cores;
}

// 實現 usage 功能
int usage(INPUT *input) {
    printf("Content-Type:application/json; charset=utf-8\n\n");
    return 0;
}