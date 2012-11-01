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

#include "polyorcutils.h"

#include  <math.h>
#include <stdlib.h>

/**
 * A int version of pow (see man pow).
 *
 * @author Oscar Norlander
 *
 * @param x
 * @param y
 *
 * @return int
 */
int intpow(int x, int y) {
    return (int) rint(pow(x, y));
}

/**
 * Frees a char ** but not its strings.
 *
 * @author Oscar Norlander
 *
 * @param arr The pointer to a char **.
 */
void free_array_of_charptr_excl(char ***arr) {
    free(*arr);
}

/**
 * Frees a char ** and its strings.
 *
 * @author Oscar Norlander
 *
 * @param arr The pointer to a char **.
 * @param len the number of strings.
 */
void free_array_of_charptr_incl(char ***arr, const int len) {
    int i;
    char **tmp = (*arr);
    for (i = 0; i < len; i++) {
        free(tmp[i]);
    }
    free(tmp);
    (*arr) = 0;
}
