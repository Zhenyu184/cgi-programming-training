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
    if (cores <= 0) {
        return 1;
    }
    return cores;
}

// 實現 usage 功能
int usage(INPUT *input) {
    printf("Content-Type:application/json; charset=utf-8\n\n");

    // 檢查輸入指標
    if (input == NULL) {
        return 0;
    }

    // 如果pid沒有或不指定就是自己
    unsigned long pid = getpid();
    INPUT *pid_tmp = NULL;
    if ((pid_tmp = CGI_Find_Parameter(input, "pid")) != NULL && pid_tmp->val && strlen(pid_tmp->val) != 0) {
        pid = (unsigned long)atoi(pid_tmp->val);
    }

    // 如果s沒有或不指定就是自己
    unsigned int s = 1;
    INPUT *s_tmp = NULL;
    if ((s_tmp = CGI_Find_Parameter(input, "s")) != NULL && s_tmp->val) {
        s = (unsigned int)atoi(s_tmp->val);
    }

    printf("pid = %lu\n", pid);

    return 0;
}