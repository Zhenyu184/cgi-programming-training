

typedef int (*callback)(int, char *[]);

int ls_func(int argc, char *argv[]) {
    return 0;
}

int lsof_func(int argc, char *argv[]) {
    return 0;
}

struct xxx {
    const char *name;
    callback fn;
} func_list[] = {
    {"ls", ls_func},
    {"lsof", losf_func},

};

int main(int argc, char *argv) {
    for (int i = 0; i < SIZEOF(func_list); ++i) {
        if (strcmp(func_list[i].name, cgi_fn_var) != 0) continue;

        return func_list[i].fn(argc, argv);
    }
}