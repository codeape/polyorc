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

/* Prints regexp errors */
void _orc_match_regerror(int errcode, const regex_t *preg, const char *pattern)
{
    size_t errbuff_len = regerror(errcode, preg, 0, 0);
    char *errbuff = malloc(errbuff_len);
    regerror(errcode, preg, errbuff, errbuff_len);
    orcerror("%s 'regexp: %s'\n", errbuff, pattern);
    free(errbuff);
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
    char *current = html;

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
        regex_t regex;
        regmatch_t pmatch[2];
        int status;

        if (0 != (status =
                  regcomp(&regex, find_pattern, REG_ICASE | REG_EXTENDED)))
        {
            _orc_match_regerror(status, &regex, find_pattern);
            regfree(&regex);
            return -1;
        }

        // Find all links
        while (REG_NOMATCH != (status = regexec(&regex, current, 2, pmatch, 0)))
        {
            if (0 != status) {
                _orc_match_regerror(status, &regex, find_pattern);
                regfree(&regex);
                return -1;
            } else {
                // Copy links and move on
                size_t str_len = (pmatch[1].rm_eo - pmatch[1].rm_so) + 1;
                char *str = malloc(str_len);
                strncpy(str, &(current[pmatch[1].rm_so]), str_len);
                str[str_len - 1] = '\0';
                current = &(current[pmatch[1].rm_eo]);

                // Apply exclude patterns
                int j = 0;
                regex_t regex_exclude;
                int status_exclude;
                int exclude = 0;
                while (excludes_len > j && !exclude) {
                    status_exclude = regcomp(&regex_exclude,
                                             excludes[j],
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

                if (exclude) {
                    orcstatus(orcm_normal, orc_warn, "exclude", "%s\n", str);
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
        }

        regfree(&regex);
        i++;
    }

    return url_count;
}
