#ifndef POLYORCMATCHER_H
#define POLYORCMATCHER_H

struct orc_match {
    char *url;
    int len;
    struct orc_match *next;
};

int find_urls(char *html, char **excludes, int excludes_len,
              int *ret_len, char ***ret);

#endif
