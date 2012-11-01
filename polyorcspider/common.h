#ifndef COMMON_H
#define COMMON_H

#include "config.h"

/* Used by main to communicate with parse_opt. */
struct arguments {
    int silent, verbose;
    char *url;
    char **excludes;
    int excludes_len;
};

#define ORC_USERAGENT ORC_NAME"/"ORC_VERSION

#endif
