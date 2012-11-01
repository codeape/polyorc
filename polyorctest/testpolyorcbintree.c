#include "testpolyorcbintree.h"
#include "polyorcbintree.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

void free_tree(void **key, void **value, const enum free_cmd cmd) {
    free(*key);
    (*key) = 0;
    if (POLY_FREE_ALL == cmd) {
        free(*value);
        (*value) = 0;
    }
}

char * create_str_key(char key) {
    char *ret = malloc(2 * sizeof(key));
    assert(0 != ret);
    ret[0] = key;
    ret[1] = '\0';
    return ret;
}

int * create_int_key(int key) {
    int *ret = malloc(sizeof(key));
    assert(0 != ret);
    (*ret) = key;
    return ret;
}

#define INT_ARR 10

void test_polyorcbintree() {
    printf("test_polyorcbintree ");

    bintree_root strroot;
    bintree_init(&strroot, bintree_streq, free_tree);

    char keys[] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',
        'p','q','r','s','t','u','v','x','y','z', 0};

    char *tmp = create_str_key('a');
    assert(0 == bintree_find(&strroot, tmp));
    free(tmp);

    bintree_add(&strroot, create_str_key('h'), create_int_key(7));
    bintree_add(&strroot, create_str_key('g'), create_int_key(6));
    bintree_add(&strroot, create_str_key('i'), create_int_key(8));

    int j = 0;
    while (0 != keys[j]) {
        char *key = create_str_key(keys[j]);
        int *val = create_int_key(j);
        int ok = bintree_add(&strroot, key, val);
        switch (keys[j]) {
        case 'h':
        case 'g':
        case 'i':
            free(key);
            free(val);
            assert(!ok);
            break;
        default:
            assert(ok);
        }
        j++;
    }

    j = 0;
    while (0 != keys[j]) {
        char *key = create_str_key(keys[j]);
        int *val = bintree_find(&strroot, key);
        free(key);
        assert(0 != val);
        assert(j == (*val));
        j++;
    }

    int *int_val;
    tmp = create_str_key('i');
    int_val = bintree_delete(&strroot, tmp);
    assert(0 != int_val);
    free(int_val);
    int_val = bintree_delete(&strroot, tmp);
    assert(0 == int_val);
    free(tmp);

    tmp = create_str_key('x');
    assert(0 != bintree_find(&strroot, tmp));
    free(tmp);

    tmp = create_str_key('g');
    assert(0 != bintree_find(&strroot, tmp));
    free(tmp);

    tmp = create_str_key('h');
    assert(0 != bintree_find(&strroot, tmp));
    free(tmp);

    tmp = create_str_key('g');
    int_val = bintree_delete(&strroot, tmp);
    assert(0 != int_val);
    free(int_val);
    free(tmp);

    tmp = create_str_key('h');
    assert(0 != bintree_find(&strroot, tmp));
    free(tmp);

    tmp = create_str_key('a');
    assert(0 != bintree_find(&strroot, tmp));
    free(tmp);

    tmp = create_str_key('z');
    assert(0 != bintree_find(&strroot, tmp));
    free(tmp);

    tmp = create_str_key('h');
    int_val = bintree_delete(&strroot, tmp);
    assert(0 != int_val);
    free(int_val);
    free(tmp);

    tmp = create_str_key('b');
    assert(0 != bintree_find(&strroot, tmp));
    free(tmp);

    tmp = create_str_key('z');
    assert(0 != bintree_find(&strroot, tmp));
    free(tmp);

    tmp = create_str_key('i');
    assert(0 == bintree_find(&strroot, tmp));
    free(tmp);

    tmp = create_str_key('g');
    assert(0 == bintree_find(&strroot, tmp));
    free(tmp);

    tmp = create_str_key('h');
    assert(0 == bintree_find(&strroot, tmp));
    free(tmp);

    bintree_free(&strroot);

    bintree_init(&strroot, bintree_inteq, free_tree);

    int intkeys[INT_ARR] = {5, 6, 4, 1, 2, 9, 7, 8, 3, 0};

    for (j = 0; j < INT_ARR; j++) {
        assert(0 != bintree_add(&strroot, create_int_key(intkeys[j]),
                    create_int_key(intkeys[j])));
    }

    for (j = 0; j < INT_ARR; j++) {
        assert(0 != bintree_find(&strroot, &(intkeys[j])));
    }

    int *int_key = create_int_key(5);
    int_val = create_int_key(5);
    assert(0 == bintree_add(&strroot, int_key, int_val));
    free(int_key);
    free(int_val);


    bintree_free(&strroot);

    printf("[ ok ]\n");
}


