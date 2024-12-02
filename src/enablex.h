typedef struct enablex_builtin {
    void * DynLoaderP;
    std::string name;
} ENABLEX_BUILTIN;

typedef struct list_info {
    char ** argv;
    int argc;
} LIST_INFO;

typedef struct builtins_list {
    ENABLEX_BUILTIN * _this;
    struct builtins_list * next;
} BUILTINS_LIST;

extern std::vector<std::shared_ptr<DynLoader>> dynloaders;
extern std::vector<std::string> names;
    
extern "C" {
    
    
    int enablex_builtin (WORD_LIST *list);
    int enablex_builtin_load (char *name);

    char *head_doc[] = {
        "Display lines from beginning of file.",
        "",
        "Copy the first N lines from the input files to the standard output.",
        "N is supplied as an argument to the `-n' option.  If N is not given,",
        "the first ten lines are copied.",
        (char *)NULL
    };
    
    struct builtin enablex_struct = {
        "enablex",			/* builtin name */
        enablex_builtin,		/* function implementing the builtin */
        BUILTIN_ENABLED,	/* initial flags for builtin */
        head_doc,		/* array of long documentation strings. */
        "enablex [-n num] [file ...]", /* usage synopsis; becomes short_doc */
        0			/* reserved for internal use */
    };


}