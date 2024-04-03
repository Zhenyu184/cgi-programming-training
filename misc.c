#include <cgi.h>
#include <stdio.h>
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

void auto_close_file(FILE **file) {
    if (file && *file) {
        fclose(*file);
        *file = NULL;
    }
}

void auto_close_cgi_input(INPUT **input) {
    if (input && *input) {
        CGI_Free_Input(*input);
        *input = NULL;
    }
}
