#include "polyorcutils.h"

#include  <math.h>
#include <stdlib.h>

int intpow(int x, int y) {
    return (int) rint(pow(x, y));
}

void free_array_of_charptr_excl(char ***arr) {
    free(*arr);
}

void free_array_of_charptr_incl(char ***arr, const int len) {
    int i;
    char **tmp = (*arr);
    for (i = 0; i < len; i++) {
        free(tmp[i]);
    }
    free(tmp);
    (*arr) = 0;
}
