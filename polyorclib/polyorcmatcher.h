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

#ifndef POLYORCMATCHER_H
#define POLYORCMATCHER_H

#include <stdlib.h>

/* De facto limit of url */
#define MAX_URL_LEN 2000

/* DNS name max */
#define SEARCH_NAME_LEN 254

/**
 * A structure that is used for collecting input when analyzing
 * html documents to gather urls.
 */
typedef struct _find_urls_input {
    char *search_name; /**< The domain part of an url */
    int search_name_len; /**< The prefix part of an url */
    char *prefix_name; /**< The prefix part of an url */
    int prefix_name_len; /**< The domain part of an url */
    char *url; /**< The curl of the analyzed html document */
    char **excludes; /**< Regex exclude patterns */
    int excludes_len; /**<  The number of exclude patterns */
    char **ret; /**< The return buffer */
    int ret_len; /**< The max number of items in the return */
} find_urls_input;

int find_search_name(const char *url, char *out, size_t out_len);

int find_prefix(const char *url, char *out, size_t out_len);

int find_urls(char *html, find_urls_input* input);

#endif
