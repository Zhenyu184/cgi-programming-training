#ifndef MAIN_H
#define MAIN_H

#include <cgi.h>

typedef int (*callback)(INPUT *);

typedef struct func_element {
    const char *name;
    callback fn;
} func_element_t;

#endif  // MAIN_H