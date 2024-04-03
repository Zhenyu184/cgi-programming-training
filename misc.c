#include <dirent.h>
#include <stdlib.h>

void auto_free_str(char **str) {
    if (str && *str) {
        free(*str);
        *str = NULL;
    }
}

void auto_close_dir(DIR **dir) {
    if (dir && *dir) {
        closedir(*dir);
        *dir = NULL;
    }
}