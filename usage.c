#include <asm/param.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct args {
    char *pid;
    int second;
} args_t;

// 定義一個耗時的工作來測試
unsigned long long work(int n) {
    if (n == 0) return 0;
    if (n == 1) return 1;
    if (n >= 2) return work(n - 2) + work(n - 1);
}

// 取得核心數
int get_cpu_cores() {
    int cores = sysconf(_SC_NPROCESSORS_ONLN);
    // sysconf 如果錯誤會返回 -1 ，並設定 errno
    // 為了避免除 0，令 cores = 0 返回 1
    if (cores <= 0)
        return 1;
    return cores;
}

// 解析 /proc/<pid>/stat，回傳現在 Tick
unsigned long parse_pid_stat(const char *pid) {
    char filepath[32] = {};
    snprintf(filepath, sizeof(filepath), "/proc/%s/stat", pid);

    // 開啟 stat 文件，如果找不到返回 0
    FILE *file = fopen(filepath, "r");
    if (file == NULL)
        return 0;

    // 讀取 stat 文件
    char buffer[2048] = {};
    if (fgets(buffer, sizeof(buffer), file) == NULL)
        return 0;

    // 解析 utime、stime、cutime、cstime
    unsigned long utime = 0, stime = 0, cutime = 0, cstime = 0;
    if (sscanf(buffer, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu %lu %lu", &utime, &stime, &cutime, &cstime) == EOF)
        return 0;

    // 關閉文件
    fclose(file);
    return utime + stime + cutime + cstime;
}

// 解析 /proc/<pid>/status，Vmsize(單位kB)
unsigned long parse_pid_status(const char *pid) {
    unsigned long ret = 0;
    char filepath[32] = {};
    snprintf(filepath, sizeof(filepath), "/proc/%s/status", pid);

    // 開啟 stat 文件，如果找不到返回 0
    FILE *file = fopen(filepath, "r");
    if (file == NULL)
        return 0;

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
    fclose(file);
    return ret;
}

// 讀取 comm
void *comm_monitor(void *args) {
    // args 傳進來是 void* 轉型成 args_t* 才找的到成員
    args_t *args_info = (args_t *)args;

    // 組路徑
    char filepath[32] = {};
    snprintf(filepath, sizeof(filepath), "/proc/%s/stat", args_info->pid);

    // 開啟 stat 文件，如果找不到返回 0
    FILE *file = fopen(filepath, "r");
    if (file == NULL)
        return NULL;

    // 讀取 stat 文件
    char buffer[2048] = {};
    if (fgets(buffer, sizeof(buffer), file) == NULL)
        return NULL;

    // 解析 comm
    char *ret = (char *)malloc(32 * sizeof(char));
    if (sscanf(buffer, "%*d %31s", ret) == EOF) {
        fclose(file);
        return NULL;
    }

    fclose(file);
    pthread_exit(ret);
}

// 監控行程 CPU 使用率
void *cpu_monitor(void *args) {
    // args 傳進來是 void* 轉型成 args_t* 才找的到成員
    args_t *args_info = (args_t *)args;

    // 開始測量
    unsigned long start_time = parse_pid_stat(args_info->pid);

    // 等待
    sleep(args_info->second);

    // 結束測量
    unsigned long end_time = parse_pid_stat(args_info->pid);

    // 推導過程
    unsigned long delta_jitters = end_time - start_time;
    double delta_process_time = (1.0 / HZ) * delta_jitters;
    double cpu_usage = (delta_process_time * 100.0) / (HZ * args_info->second * get_cpu_cores()) * 100.0;

    double *ret = (double *)malloc(sizeof(double));
    *ret = cpu_usage;
    pthread_exit(ret);
}

// 監控行程 Memory 使用率
void *memory_monitor(void *args) {
    // args 傳進來是 void* 轉型成 args_t* 才找的到成員
    args_t *args_info = (args_t *)args;

    // 等待
    sleep(args_info->second);

    // 測量
    unsigned long size = parse_pid_status(args_info->pid);

    unsigned long *ret = (unsigned long *)malloc(sizeof(unsigned long));
    *ret = size;
    pthread_exit(ret);
}

// 實現 usage 功能
void usage(char *pid, char *second) {
    // 定義參數結構
    args_t args = {.pid = pid,
                   .second = atoi(second)};
    args_t *args_ptr = &args;

    // 開執行緒偵測
    pthread_t comm_thread, cpu_thread, memory_thread;
    if (pthread_create(&comm_thread, NULL, comm_monitor, (void *)args_ptr) != 0)
        return;
    if (pthread_create(&cpu_thread, NULL, cpu_monitor, (void *)args_ptr) != 0)
        return;
    if (pthread_create(&memory_thread, NULL, memory_monitor, (void *)args_ptr) != 0)
        return;

    // 測試用
    // printf("work = %llu\n", work(50));

    // 結束執行續
    char *comm_ret = NULL;
    double *cpu_ret = NULL;
    unsigned long *memory_ret = NULL;
    if (pthread_join(comm_thread, (void **)&comm_ret) != 0)
        return;
    if (pthread_join(cpu_thread, (void **)&cpu_ret) != 0)
        return;
    if (pthread_join(memory_thread, (void **)&memory_ret) != 0)
        return;

    // 印出 json
    printf("{");
    printf("\"comm\":\"%s\",", comm_ret);
    printf("\"cpu(%%)\":%lf,", *cpu_ret);
    printf("\"memory\":%lu", *memory_ret);
    printf("}");

    free(comm_ret);
    free(cpu_ret);
    free(memory_ret);

    return;
}