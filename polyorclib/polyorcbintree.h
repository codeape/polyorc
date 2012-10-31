#ifndef POLYORCBINTREE_H
#define POLYORCBINTREE_H

struct bintree_node;

enum free_cmd {
    POLY_DELETE,
    POLY_FREE_ALL
};

typedef int (*bintree_equal_fptr)(void *, void *);
typedef void (*bintree_free_fptr)(void **, void **, const enum free_cmd);

struct bintree_node {
    void *key;
    void *value;
    struct bintree_node *less_n;
    struct bintree_node *more_n;
};

int bintree_streq(void *current, void *added);
int bintree_inteq(void *current, void *added);

struct bintree_root {
    int node_count;
    struct bintree_node *_root_node;
    bintree_equal_fptr _equals;
    bintree_free_fptr _free_value;
};

void bintree_init(struct bintree_root *,
                  bintree_equal_fptr,
                  bintree_free_fptr);

int bintree_add(struct bintree_root *root, void *key, void *value);

void * bintree_find(struct bintree_root *root, void *key);

void * bintree_delete(struct bintree_root *root, void *key);

void bintree_free(struct bintree_root *root);

#endif
