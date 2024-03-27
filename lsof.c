#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void lsof(const char *path) {
    printf("path = %s\n", path);
    return;
}