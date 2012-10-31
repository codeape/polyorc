#include "polyorcbintree.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

int bintree_streq(void *current, void *added) {
    // We check if the added is bigger than current (see strcmp)
    return strcmp((const char*)added, (const char*)current);
}

int bintree_inteq(void *current, void *added) {
    // We check if the added is bigger than current (see strcmp)
    int int_added = *((int *)added);
    int int_current = *((int *)current);
    return int_added - int_current;
}

void bintree_init(struct bintree_root *root,
                  bintree_equal_fptr equals,
                  bintree_free_fptr free_value) {
    memset(root, 0, sizeof(*root));
    root->_equals = equals;
    root->_free_value = free_value;
}

int _bintree_add(bintree_equal_fptr eq, struct bintree_node **node,
                  void *key, void *value) {
    if (0 == (*node)) {
        struct bintree_node * tmp = malloc(sizeof(**node));
        memset(tmp, 0, sizeof(**node));
        tmp->key = key;
        tmp->value = value;
        (*node) = tmp;
        return 1;
    }

    int compare = eq((*node)->key, key);
    if (0 < compare) {
        // bigger
        return _bintree_add(eq, &((*node)->more_n), key, value);
    } else if (0 > compare) {
        // smaller
        return _bintree_add(eq, &((*node)->less_n), key, value);
    } else {
        return 0;
    }
}

int bintree_add(struct bintree_root *root, void *key, void *value) {
    int ret = _bintree_add(root->_equals, &(root->_root_node), key, value);
    if (0 != ret) {
        root->node_count++;
    }
    return ret;
}

void * _bintree_find(bintree_equal_fptr eq, struct bintree_node *node,
                    void *key) {
    if (0 == node) {
        return 0;
    }

    int compare = eq(node->key, key);
    if (0 < compare) {
        // bigger
        return _bintree_find(eq, node->more_n, key);
    } else if (0 > compare) {
        // smaller
        return _bintree_find(eq, node->less_n, key);
    } else {
        return node->value;
    }
}

void * bintree_find(struct bintree_root *root, void *key) {
    return _bintree_find(root->_equals, root->_root_node, key);
}


void _bintree_reenter(bintree_equal_fptr eq, struct bintree_node **root,
                      struct bintree_node *node) {
    if (0 == node) {
        return;
    }
    if (0 == (*root)) {
        (*root) = node;
        return;
    }

    int compare = eq((*root)->key, node->key);
    if (0 < compare) {
        // bigger
        _bintree_reenter(eq, &((*root)->more_n), node);
    } else if (0 > compare) {
        // smaller
        _bintree_reenter(eq, &((*root)->less_n), node);
    } else {
        // Uggly yes, but we do not allow insert for equal keys so this
        // should never happen.
        assert(0 != compare);
    }
}

void * _bintree_delete(bintree_equal_fptr eq, bintree_free_fptr dl,
                      struct bintree_node **node, void *key, int *ncount) {
    if (0 == (*node)) {
        return 0;
    }

    int compare = eq((*node)->key, key);
    if (0 < compare) {
        // bigger
        return _bintree_delete(eq, dl, &((*node)->more_n), key, ncount);
    } else if (0 > compare) {
        // smaller
        return _bintree_delete(eq, dl, &((*node)->less_n), key, ncount);
    } else {
        struct bintree_node *tmp = (*node);
        void *ret = tmp->value;
        dl(&(tmp->key),&(tmp->value), POLY_DELETE);
        (*node) = tmp->less_n;
        _bintree_reenter(eq, node, tmp->more_n);
        free(tmp);
        (*ncount) = (*ncount) - 1;
        return ret;
    }
}

void * bintree_delete(struct bintree_root *root, void *key) {
    return _bintree_delete(root->_equals, root->_free_value,
                         &(root->_root_node), key, &(root->node_count));
}

void _bintree_free(struct bintree_node **node, bintree_free_fptr dl) {
    if (0 != (*node)) {
        _bintree_free(&((*node)->less_n), dl);
        _bintree_free(&((*node)->more_n), dl);
        dl(&((*node)->key), &((*node)->value), POLY_FREE_ALL);
        free(*node);
        (*node) = 0;
    }
}

void bintree_free(struct bintree_root *root) {
    _bintree_free(&(root->_root_node), root->_free_value);
    memset(root, 0, sizeof(*root));
}
