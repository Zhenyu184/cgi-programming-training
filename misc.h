#ifndef MISC_H
#define MISC_H

#include <dirent.h>

#ifndef AUTO_STR
#define AUTO_STR __attribute__((cleanup(auto_free_str))) char *
void auto_free_str(char **str);
#endif

#ifndef AUTO_DIR
#define AUTO_DIR __attribute__((cleanup(auto_close_dir))) DIR *
void auto_close_dir(DIR **dir);
#endif

#endif  // MISC_H