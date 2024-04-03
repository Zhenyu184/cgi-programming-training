#ifndef MISC_H
#define MISC_H

#include <cgi.h>
#include <dirent.h>

#define SIZEOF(A) (sizeof(A) / sizeof(A[0]))

#ifndef AUTO_STR
#define AUTO_STR __attribute__((cleanup(auto_free_str))) char *
void auto_free_str(char **str);
#endif

#ifndef AUTO_DIR
#define AUTO_DIR __attribute__((cleanup(auto_close_dir))) DIR *
void auto_close_dir(DIR **dir);
#endif

#ifndef AUTO_FILE
#define AUTO_FILE __attribute__((cleanup(auto_close_file))) FILE *
void auto_close_file(FILE **file);
#endif

#ifndef AUTO_CGI_INPUT
#define AUTO_CGI_INPUT __attribute__((cleanup(auto_close_cgi_input))) INPUT *
void auto_close_cgi_input(INPUT **input);
#endif

#endif  // MISC_H