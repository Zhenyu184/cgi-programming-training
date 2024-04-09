#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

// 解析 /proc/.../stat 文件
char *parse_stat(unsigned long pid, unsigned long *cpu_time) {
    // 組路徑
    char filepath[32] = {};
    snprintf(filepath, sizeof(filepath), "/proc/%lu/stat", pid);

    // 開啟 stat 文件，如果找不到返回 0
    AUTO_FILE file = fopen(filepath, "r");
    if (file == NULL) {
        return NULL;
    }

    // 讀取 stat 文件
    char buffer[2048] = {};
    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        return NULL;
    }

    // 解析 comm、utime、stime、cutime、cstime
    char *ret = (char *)malloc(32 * sizeof(char));
    unsigned long utime = 0, stime = 0, cutime = 0, cstime = 0;
    if (sscanf(buffer, "%*d %31s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu %lu %lu", ret, &utime, &stime, &cutime, &cstime) == EOF) {
        return NULL;
    }

    // 計算 cpu time
    *cpu_time = utime + stime + cutime + cstime;
    return ret;
}

// 解析 /proc/.../status 文件
unsigned long parse_status(unsigned long pid) {
    unsigned long ret = 0;
    char filepath[32] = {};
    snprintf(filepath, sizeof(filepath), "/proc/%lu/status", pid);

    // 開啟 stat 文件，如果找不到返回 0
    AUTO_FILE file = fopen(filepath, "r");
    if (file == NULL) {
        return 0;
    }

    // 讀取 stat 文件
    char buffer[128] = {};
    char *value = NULL;
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        value = strstr(buffer, "VmSize");
        // 找到 "VmSize"
        if (value != NULL) {
            sscanf(value, "%*s %lu", &ret);
            break;
        }
    }

    // 關閉文件
    return ret;
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

    // 取 comm 和 start time 資訊
    unsigned long cpu_time = 0;
    char *comm = parse_stat(pid, &cpu_time);
    if (comm == NULL) {
        return 0;
    }

    // 紀錄 start time 資訊
    unsigned long start_time = cpu_time;

    // 等待
    sleep(s);

    // 取 emd time 資訊
    if (parse_stat(pid, &cpu_time) == NULL) {
        return 0;
    }
    unsigned long end_time = cpu_time;

    // 計算 cpu 百分比
    double delta_process_time = (1.0 / sysconf(_SC_CLK_TCK)) * (end_time - start_time);
    double cpu_usage = (delta_process_time * 100.0) / (sysconf(_SC_CLK_TCK) * s * get_cpu_cores()) * 100.0;

    // 計算 memory 使用量
    unsigned long memory_usage = parse_status(pid);

    // 印出 Json
    printf(
        "{"
        "\"comm\":\"%s\","
        "\"cpu(percentage)\":%.2lf,"
        "\"memory(KB)\":%ld"
        "}",
        comm,
        cpu_usage,
        memory_usage);

    return 0;
}