#ifndef COMMON_H
#define COMMON_H

/* Used by main to communicate with parse_opt. */
struct arguments {
    int silent, verbose;
    char *url;
    char **excludes;
    int excludes_len;
};

#endif
