

#include <asm/param.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "add.h"
#include "cJSON.h"
#include "ls.h"

typedef struct args_struct {
    char *pid;
    int second;
    cJSON *ret_json;
} args_t;

// 錯誤處理
void error_handling() {
}

// 如果該段落是目標則回傳 value，不是則是 NULL
char *parse_value(char *section, const char *target) {
    char *equalPtr = strchr(section, '=');

    // 如果段落沒有 "=" 提早 return
    if (equalPtr == NULL)
        return NULL;

    // 將"="的"值"改成結束字元
    *equalPtr = '\0';
    char *key = section;
    char *value = equalPtr + 1;

    // 比較 key 是否為要找的目標
    if (strcmp(key, target) == 0)
        return value;

    // 什麼都沒找到
    return NULL;
}

// 解析整個參數字串輸出目標 key 的 value
char *parse_parameter(char *str, const char *target) {
    // 複製輸入字串指標，避免副作用
    char *str_copy = strdup(str);

    const char *delimiter = "&";
    char *ret = NULL;
    char *temp = strtok(str_copy, delimiter);
    while (temp != NULL) {
        // 如果該段落是要找的目標提早 return
        if (ret = parse_value(temp, target))
            return ret;

        // 繼續找
        temp = strtok(NULL, delimiter);
    }

    // 什麼都沒匹配到
    return NULL;
}

// 檢查字串是否為正整數
bool is_integer(const char *num) {
    int i = 0;

    // 歷遍每個字元
    for (i = 0; num[i]; ++i) {
        if (num[i] > '9' || num[i] < '0')
            return false;
    }

    // 都是數字但超過 10 位數(pid_t是int)
    if (i > 10)
        return false;
    return true;
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

// 定義一個耗時的工作來測試
unsigned long long work(int n) {
    if (n == 0) return 0;
    if (n == 1) return 1;
    if (n >= 2) return work(n - 2) + work(n - 1);
}

// 解析 /proc/<pid>/stat，現在 Tick
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

    // printf("HZ = %d\n", HZ);
    // printf("1 個 Tick = %lf 秒\n", 1.0 / HZ); // Tick 是 HZ 的倒數
    // printf("經過 %lu 個 Tick\n", delta_jitters);
    // printf("所以共經過 %lf 秒\n", delta_process_time);
    // printf("CPU 使用率是 %lf %% \n", cpu_usage);
    cJSON_AddNumberToObject(args_info->ret_json, "cpu", cpu_usage);

    pthread_exit(&cpu_usage);
}

// 監控行程 Memory 使用率
void *memory_monitor(void *args) {
    // args 傳進來是 void* 轉型成 args_t* 才找的到成員
    args_t *args_info = (args_t *)args;

    // 等待
    sleep(args_info->second);

    // 測量
    unsigned long size = parse_pid_status(args_info->pid);
    cJSON_AddNumberToObject(args_info->ret_json, "memory", size);

    pthread_exit(&size);
}

// 實現 top 功能
void top(args_t *args) {
    // 開執行緒偵測
    pthread_t cpu_thread, memory_thread;
    if (pthread_create(&cpu_thread, NULL, cpu_monitor, (void *)args) != 0)
        return;
    if (pthread_create(&memory_thread, NULL, memory_monitor, (void *)args) != 0)
        return;

    // 測試用
    // printf("work = %llu\n", work(50));

    // 結束執行續
    pthread_join(cpu_thread, NULL);
    pthread_join(memory_thread, NULL);

    // 印出 json
    printf("%s\n", cJSON_Print(args->ret_json));

    return;
}

int main(int argc, char *argv) {
    printf("Content-Type:application/json; charset=utf-8\n\n");

    // printf("Hello CGI\n");
    printf("Sum: %d\n", add(3, 4));
    printf("ls: %d\n", ls(5, 6));
    return 0;

    do {
        // 讀取參數到 para
        char *para = getenv("QUERY_STRING");
        if (para == NULL) {
            error_handling();
            break;
        }

        // 讀取目標 fn 到 fn
        char *fn = parse_parameter(para, "fn");
        if (fn == NULL) {
            error_handling();
            break;
        }

        // 讀取目標 file 到 filepath
        char *filepath = parse_parameter(para, "file");

        // 讀取目標 s 到 second
        char *second = parse_parameter(para, "s");

        // 讀取目標 pid 到 pid
        char *pid = parse_parameter(para, "pid");

        // 如果 pid 或 second 不是正整數
        if (!is_integer(pid) || !is_integer(second)) {
            error_handling();
            break;
        }

        // 如果 fn 為空，就退出
        if (strlen(fn) == 0) {
            break;
        }

        // 如果 pid 為空，就預設為自己
        if (strlen(pid) == 0) {
            pid = "self";
        }

        // 如果 second 為空，就假設 0
        if (strlen(second) == 0) {
            second = "0";
        }

        // 參數宣告、賦值
        args_t args = {.second = atoi(second),
                       .pid = pid,
                       .ret_json = cJSON_CreateObject()};
        args_t *args_ptr = &args;

        // To do...
        top(args_ptr);
    } while (false);

    return 0;
}
