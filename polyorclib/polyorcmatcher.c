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

#include "polyorcmatcher.h"
#include "polyorcout.h"

#include <stdlib.h>
#include <sys/types.h>
#include <regex.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Prints regexp errors */
void _orc_match_regerror(int errcode, const regex_t *preg, const char *pattern)
{
    size_t errbuff_len = regerror(errcode, preg, 0, 0);
    char *errbuff = malloc(errbuff_len);
    regerror(errcode, preg, errbuff, errbuff_len);
    orcerror("%s 'regexp: %s'\n", errbuff, pattern);
    free(errbuff);
}

int find_search_name(const char *url, char *out, size_t out_len) {
    const char *domain_pattern = "[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9]\\.[a-zA-Z]{2,6}$";
    const char *start_url = url;

    regex_t regex;
    regmatch_t pmatch[1];
    int status;

    /* Strip protocol part */
    if ('h' == url[0] && 't' == url[1] && 't' == url[2] && 'p' == url[3] &&
        ':' == url[4] && '/' == url[5] && '/' == url[6])
    {
        /* http */
        start_url += 7;
    } else if ('h' == url[0] && 't' == url[1] && 't' == url[2] &&
               'p' == url[3] && 's' == url[4] && ':' == url[5] &&
               '/' == url[6] && '/' == url[7])
    {
        /* https */
        start_url += 8;
    }

    /* Strip everything but the name name */
    char *name = 0;
    size_t name_len = 0;
    if (0 != (name = index(start_url, '/')) && name != start_url) {
        name_len = (name - start_url) + 1;
        name = calloc(name_len, sizeof(char));
        strncpy(name, start_url, name_len);
        name[name_len - 1] = '\0';
    } else {
        name_len = strlen(start_url) + 1;
        name = calloc(name_len, sizeof(char));
        strncpy(name, start_url, name_len);
    }

    /* check local host, name name and ip */
    if (0 == strcmp(name, "localhost")) {
        strncpy(out, name, out_len);
        free(name);
        return 1;
    }

    struct sockaddr_in sa;
    if (inet_pton(AF_INET, name, &(sa.sin_addr)) ||
        inet_pton(AF_INET6, name, &(sa.sin_addr)))
    {
        strncpy(out, name, out_len);
        free(name);
        return 1;
    }

    if (0 != (status = regcomp(&regex, domain_pattern,
                               REG_ICASE | REG_EXTENDED)))
    {
        _orc_match_regerror(status, &regex, domain_pattern);
        regfree(&regex);
        return -1;
    }

    if (REG_NOMATCH != (status = regexec(&regex, name, 2, pmatch, 0))) {
        if (0 != status) {
            _orc_match_regerror(status, &regex, domain_pattern);
            regfree(&regex);
            return -1;
        }
        strncpy(out, &(name[pmatch[0].rm_so]), out_len);
        free(name);
        return 1;
    }

    return 0;
}

/**
 * Searches a buffer for html links by looking for href and src
 * attributes.
 *
 * @author Oscar Norlander
 *
 * @param html The buffer containing html.
 * @param excludes Regex exclude patterns
 * @param excludes_len The number of exclude patterns
 * @param ret The return buffer.
 * @param ret_len The max number of returns the return buffer
 *                currently suport.*
 *
 * @return int The number of urls found.
 */
int find_urls(char *html, char **excludes, int excludes_len,
              char ***ret, int *ret_len)
{
    const char *find_patterns[] = {
        "href[:space:]*=[:space:]*\"[:space:]*(/[^\"]*)\"",
        "href[:space:]*=[:space:]*'[:space:]*(/[^']*)'",
        "src[:space:]*=[:space:]*\"[:space:]*(/[^\"]*)\"",
        "src[:space:]*=[:space:]*'[:space:]*(/[^']*)'",
        0
    };

    const char *find_pattern = 0;
    int i = 0;
    int url_count = 0;
    while (0 != (find_pattern = find_patterns[i])) {
        /* Last match pointer that points directly after last match */
        char *current = html; /* Restet last match pointer */

        regex_t regex;
        regmatch_t pmatch[2];
        int status;

        if (0 != (status = regcomp(&regex, find_pattern,
                                   REG_ICASE | REG_EXTENDED)))
        {
            _orc_match_regerror(status, &regex, find_pattern);
            regfree(&regex);
            return -1;
        }

        /* Find all links */
        while (REG_NOMATCH != (status = regexec(&regex, current, 2, pmatch, 0)))
        {
            if (0 != status) {
                _orc_match_regerror(status, &regex, find_pattern);
                regfree(&regex);
                return -1;
            }
            /* Copy links and move on */
            size_t str_len = (pmatch[1].rm_eo - pmatch[1].rm_so) + 1;
            char *str = malloc(str_len);
            strncpy(str, &(current[pmatch[1].rm_so]), str_len);
            str[str_len - 1] = '\0';
            current = &(current[pmatch[1].rm_eo]);

            /* Apply exclude patterns */
            int j = 0;
            regex_t regex_exclude;
            int status_exclude;
            int exclude = 0;
            while (excludes_len > j && !exclude) {
                status_exclude = regcomp(&regex_exclude, excludes[j],
                                         REG_EXTENDED);
                if (0 != status_exclude) {
                    _orc_match_regerror(status_exclude, &regex_exclude,
                                        find_pattern);
                    regfree(&regex_exclude);
                    regfree(&regex);
                    free(str);
                    return -1;
                }
                status_exclude = regexec(&regex_exclude, str, 0, 0, 0);
                if (0 == status_exclude) {
                    exclude = 1;
                } else if (REG_NOMATCH != status_exclude) {
                    _orc_match_regerror(status_exclude, &regex, find_pattern);
                    regfree(&regex_exclude);
                    regfree(&regex);
                    free(str);
                    return -1;
                }
                regfree(&regex_exclude);
                j++;
            }

            /* Execute include or exclude */
            if (exclude) {
                orcstatus(orcm_normal, orc_yellow, "exclude", "%s\n", str);
                free(str);
            } else {
                url_count++;
                char **tmp = 0;
                if (url_count  > (*ret_len)) {
                    tmp = realloc((*ret), url_count * sizeof(**ret));
                    if (0 == tmp) {
                        orcerror("%s (%d)\n", strerror(errno), errno);
                        regfree(&regex);
                        free(str);
                        return -1;
                    }
                    (*ret_len) = url_count;
                    (*ret) = tmp;
                } else {
                    tmp = (*ret);
                }
                tmp[url_count - 1] = str;
            }
        }
        regfree(&regex);
        i++;
    }

    return url_count;
}
