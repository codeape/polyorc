/*
 Originally created by Oscar Norlander (codeape)
 ...................................................................
 @@@@@@@    @@@@@@   @@@       @@@ @@@   @@@@@@   @@@@@@@    @@@@@@@
 @@@@@@@@  @@@@@@@@  @@@       @@@ @@@  @@@@@@@@  @@@@@@@@  @@@@@@@@
 @@!  @@@  @@!  @@@  @@!       @@! !@@  @@!  @@@  @@!  @@@  !@@
 !@!  @!@  !@!  @!@  !@!       !@! @!!  !@!  @!@  !@!  @!@  !@!
 @!@@!@!   @!@  !@!  @!!        !@!@!   @!@  !@!  @!@!!@!   !@!
 !!@!!!    !@!  !!!  !!!         @!!!   !@!  !!!  !!@!@!    !!!
 !!:       !!:  !!!  !!:         !!:    !!:  !!!  !!: :!!   :!!
 :!:       :!:  !:!   :!:        :!:    :!:  !:!  :!:  !:!  :!:
  ::       ::::: ::   :: ::::     ::    ::::: ::  ::   :::   ::: :::
  :         : :  :   : :: : :     :      : :  :    :   : :   :: :: :
 ...................................................................
 Polyorc is under BSD 2-Clause License (see LICENSE file)
*/

#ifndef POLYORCBINTREE_H
#define POLYORCBINTREE_H

struct bintree_node;

enum free_cmd {
    POLY_DELETE,
    POLY_FREE_ALL
};

typedef int (*bintree_equal_fptr)(void *, void *);
typedef void (*bintree_free_fptr)(void **, void **, const enum free_cmd);

typedef struct _bintree_node {
    void *key;
    void *value;
    struct _bintree_node *less_n;
    struct _bintree_node *more_n;
} bintree_node;

typedef struct _bintree_root {
    int node_count;
    bintree_node *_root_node;
    bintree_equal_fptr _equals;
    bintree_free_fptr _free_value;
} bintree_root;

int bintree_streq(void *current, void *added);
int bintree_inteq(void *current, void *added);

void bintree_init(bintree_root *root,
                  bintree_equal_fptr equals,
                  bintree_free_fptr free_value);

int bintree_add(bintree_root *root, void *key, void *value);

void * bintree_find(bintree_root *root, void *key);

void * bintree_delete(bintree_root *root, void *key);

void bintree_free(bintree_root *root);

#endif
