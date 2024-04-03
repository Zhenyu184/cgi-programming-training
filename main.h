#ifndef MAIN_H
#define MAIN_H

typedef int (*callback)(int);

typedef struct func_element {
    const char *name;
    callback fn;
} func_element_t;

typedef struct args {
    char *fn;
    char *file;
    int pid;
    int s;
} args_t;

#endif