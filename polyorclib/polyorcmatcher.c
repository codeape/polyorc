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
#include <arpa/inet.h>
#include <stdio.h>
#include <uriparser/Uri.h>

/* Prints regexp errors */
void _orc_match_regerror(int errcode, const regex_t *preg, const char *pattern)
{
    size_t errbuff_len = regerror(errcode, preg, 0, 0);
    char *errbuff = calloc(errbuff_len, sizeof(char));
    regerror(errcode, preg, errbuff, errbuff_len);
    orcerror("%s 'regexp: %s'\n", errbuff, pattern);
    free(errbuff);
}

/**
 * Identifies and copies the domain part of an url. Example:
 * input "http://www.example.com/index.html" outputs
 * "example.com".
 *
 * @author Oscar Norlander
 *
 * @param url The url to analyze.
 * @param out A output buffer to put the result in.
 * @param out_len The lengt of the output buffer.
 *
 * @return int 1 on succes 0 on fail
 */
int find_search_name(const char *url, char *out, size_t out_len) {
    regex_t regex;
    regmatch_t pmatch[2]; // We use 2 to avoid memleak
    int status;
    const char *pattern = "([a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9]\\.[a-zA-Z]{2,6})$";
    const char *start_url = url;

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

    /* Strip everything but the name */
    char *name = 0;
    size_t name_len = 0;
    if (0 != (name = index(start_url, ':')) && name != start_url) {
        name_len = (name - start_url) + 1;
        name = calloc(name_len, sizeof(char));
        strncpy(name, start_url, name_len);
        name[name_len - 1] = '\0';
    } else if (0 != (name = index(start_url, '/')) && name != start_url) {
        name_len = (name - start_url) + 1;
        name = calloc(name_len, sizeof(char));
        strncpy(name, start_url, name_len);
        name[name_len - 1] = '\0';
    } else {
        name_len = strlen(start_url) + 1;
        name = calloc(name_len, sizeof(char));
        strncpy(name, start_url, name_len);
    }
    if (0 == name) {
        orcerror("%s (%d)\n", strerror(errno), errno);
        return 0;
    }

    /* check local host, name and ip */
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

    if (0 != (status = regcomp(&regex, pattern, REG_ICASE | REG_EXTENDED)))
    {
        _orc_match_regerror(status, &regex, pattern);
        regfree(&regex);
        return 0;
    }

    if (REG_NOMATCH != (status = regexec(&regex, name, 2, pmatch, 0))) {
        if (0 != status) {
            _orc_match_regerror(status, &regex, pattern);
            free(name);
            regfree(&regex);
            return 0;
        }
        strncpy(out, &(name[pmatch[0].rm_so]), out_len);
        free(name);
        regfree(&regex);
        return 1;
    }

    regfree(&regex);
    return 0;
}

/* Applies the exclude patterns
   returns 1 == exclude
           0 == include
          -1 == error
   */
int is_excluded(const char* url, const find_urls_input* input) {
    /* Apply exclude patterns */
    int j = 0;
    regex_t regex_exclude;
    int status_exclude;
    int exclude = 0;
    while (input->excludes_len > j && !exclude) {
        status_exclude = regcomp(&regex_exclude, input->excludes[j],
                                 REG_EXTENDED);
        if (0 != status_exclude) {
            _orc_match_regerror(status_exclude, &regex_exclude,
                                input->excludes[j]);
            regfree(&regex_exclude);
            return -1;
        }
        status_exclude = regexec(&regex_exclude, url, 0, 0, 0);
        if (0 == status_exclude) {
            exclude = 1;
        } else if (REG_NOMATCH != status_exclude) {
            _orc_match_regerror(status_exclude, &regex_exclude,
                                input->excludes[j]);
            regfree(&regex_exclude);
            return -1;
        }
        regfree(&regex_exclude);
        j++;
    }
    return exclude;
}

/* Fixes urls by adding missing parts like domain and making them absolute.
   returns 1 == exclude
           0 == include
          -1 == error
*/
int fix_url(char **url, const find_urls_input* input ) {
    UriParserStateA state;
    UriUriA absolute_dest;
    UriUriA relative_source;
    UriUriA absolute_base;

    /* make str to uri */
    state.uri = &relative_source;
    if (uriParseUriA(&state, (*url)) != URI_SUCCESS) {
        uriFreeUriMembersA(&relative_source);
        return 0;
    }

    /* make str to uri */
    state.uri = &absolute_base;
    if (uriParseUriA(&state, input->url) != URI_SUCCESS) {
        uriFreeUriMembersA(&relative_source);
        uriFreeUriMembersA(&absolute_base);
        return 0;
    }

    /* make realative url to absolute url etc.
       relative_source holds for example "../TWO" now
       absolute_base holds for example "http://example.com/one/two/three" now
     */
    if (uriAddBaseUriA(&absolute_dest, &relative_source, &absolute_base) != URI_SUCCESS) {
        uriFreeUriMembersA(&absolute_dest);
        uriFreeUriMembersA(&relative_source);
        uriFreeUriMembersA(&absolute_base);
        return 0;
    }
    /* absolute_dest holds for example "http://example.com/one/TWO" now */

    char * uriString;
    int charsRequired;

    /* Get length of new url */
    if (uriToStringCharsRequiredA(&absolute_dest, &charsRequired) != URI_SUCCESS) {
        uriFreeUriMembersA(&absolute_dest);
        uriFreeUriMembersA(&relative_source);
        uriFreeUriMembersA(&absolute_base);
        return 0;
    }
    charsRequired++;

    /* Get new url */
    uriString = calloc(charsRequired, sizeof(char));
    if (uriString == NULL) {
        uriFreeUriMembersA(&absolute_dest);
        uriFreeUriMembersA(&relative_source);
        uriFreeUriMembersA(&absolute_base);
        return 0;
    }
    if (uriToStringA(uriString, &absolute_dest, charsRequired, NULL) != URI_SUCCESS) {
        uriFreeUriMembersA(&absolute_dest);
        uriFreeUriMembersA(&relative_source);
        uriFreeUriMembersA(&absolute_base);
        free(uriString);
        return 0;
    }
    free(*url);
    (*url) = uriString;
    uriFreeUriMembersA(&absolute_dest);
    uriFreeUriMembersA(&relative_source);
    uriFreeUriMembersA(&absolute_base);

    /* Count dots in domain */
    int i = 0;
    int append = 0;
    for (i=0; i < input->search_name_len; i++) {
        if ('.' == input->search_name[i]) {
            append++;
        }
    }
    /* Terminate the dots */
    int name_len = input->search_name_len + append + 1;
    char *name = calloc(name_len , sizeof(char));
    i = 0;
    int j = 0;
    while (j < input->search_name_len) {
        if ('.' == input->search_name[j]) {
            name[i] = '\\';
            i++;
        }
        name[i] = input->search_name[j];
        i++;
        j++;
    }
    name[name_len - 1] = '\0';

    /* Create search pattern for domain url */
    const char *pattern_format =
        "%s(:[[:digit:]]{1,5})?/.*|%s(:[[:digit:]]{1,5})?$\0";
    int find_pattern_len = strlen(pattern_format) + (name_len * 2) + 1;
    char *find_pattern = calloc(find_pattern_len, sizeof(char));
    snprintf(find_pattern, find_pattern_len, pattern_format, name, name);
    free(name);

    regex_t regex;
    regmatch_t pmatch[2];
    int status;

    /* Decide if this url is intra domain */
    if (0 != (status = regcomp(&regex, find_pattern,
                               REG_ICASE | REG_EXTENDED)))
    {
        _orc_match_regerror(status, &regex, find_pattern);
        regfree(&regex);
        return -1;
    }
    if (REG_NOMATCH != (status = regexec(&regex, (*url), 2, pmatch, 0))) {
        if (0 != status) {
            _orc_match_regerror(status, &regex, find_pattern);
            regfree(&regex);
            free(find_pattern);
            return -1;
        }
        free(find_pattern);
        regfree(&regex);
        /* Yes, the url is intra domain url */
        return 1;
    }
    free(find_pattern);
    regfree(&regex);


    /* No, the url is not intra domain url */
    return 0;
}

/**
 * Searches a buffer for html links by looking for href and src
 * attributes.
 *
 * @author Oscar Norlander
 *
 * @param html The buffer containing html.
 * @param input See find_urls_input.
 *
 * @return int The number of urls found.
 */
int find_urls(char *html, find_urls_input* input)
{
    const char *find_patterns[] = {
        "href[:space:]*=[:space:]*\"[:space:]*([^\"]*)\"",
        "href[:space:]*=[:space:]*'[:space:]*([^']*)'",
        "src[:space:]*=[:space:]*\"[:space:]*([^\"]*)\"",
        "src[:space:]*=[:space:]*'[:space:]*([^']*)'",
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
            char *str = calloc(str_len, sizeof(char));
            strncpy(str, &(current[pmatch[1].rm_so]), str_len);
            str[str_len - 1] = '\0';
            current = &(current[pmatch[1].rm_eo]);

            int exclude = 1;
            if (0 != (status = fix_url(&str, input))) {
                if (-1 == status) {
                    regfree(&regex);
                    return -1;
                }
                exclude = is_excluded(str, input);
                if (-1 == exclude) {
                    regfree(&regex);
                    free(str);
                    return -1;
                }
            }

            /* Execute include or exclude */
            if (exclude) {
                orcstatus(orcm_verbose, orc_yellow, "exclude", "%s\n", str);
                free(str);
            } else {
                /* Add to result */
                url_count++;
                char **tmp = 0;
                if (url_count  > input->ret_len) {
                    tmp = realloc((input->ret), url_count * sizeof(char *));
                    if (0 == tmp) {
                        orcerror("%s (%d)\n", strerror(errno), errno);
                        regfree(&regex);
                        free(str);
                        return -1;
                    }
                    input->ret_len = url_count;
                    input->ret = tmp;
                } else {
                    tmp = input->ret;
                }
                tmp[url_count - 1] = str;
            }
        }
        regfree(&regex);
        i++;
    }

    return url_count;
}
