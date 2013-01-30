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

#include "testpolyorcmatcher.h"
#include "polyorcmatcher.h"
#include "polyorcutils.h"

#include <string.h>
#include <stdio.h>

void test_polyorcmatcher() {

    char *html = 
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"\
        "<!DOCTYPE html \n"\
        "        PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \n"\
        "        \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"\
        "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\""\
        "      lang=\"en\">\n"\
        "<head>\n"\
        "<style type=\"text/css\">\n"\
        "body {\n"\
        "        margin:5px;\n"\
        "        padding:5px;\n"\
        "}\n"\
        ".center {\n"\
        "        margin-left:auto;\n"\
        "        margin-right:auto;\n"\
        "        width:600px;\n"\
        "        text-align:left;\n"\
        "}\n"\
        "</style>\n"\
        "<title>null</title>\n"\
        "</head>\n"\
        "<body> \n"\
        "<a href=\"/blog/index.html\">a</a>\n"\
        "<a href=\"../ego/index.html\">b</a>\n"\
        "<a href=\"bog/index.html\">c</a>\n"\
        "<a href=\"http://www.example.com/big/index.html\">d</a>\n"\
        "<a href=\"http://www.example.com\">e</a>\n"\
        "<a href=\"www.example.com\">e</a>\n"\
        "<a href=\"example.com\">e</a>\n"\
        "<a href=\"http://www.example.com:80/\">e</a>\n"\
        "<a href=\"http://www.example.com:8080\">e</a>\n"\
        "<a href=\"http://127.0.0.1/\">e</a>\n"\
        "<a href=\"http://127.0.0.1\">e</a>\n"\
        "<a href=\"127.0.0.1\">e</a>\n"\
        "<a href=\"http://localhost\">e</a>\n"\
        "<a href=\"localhost\">e</a>\n"\
        "<a href=\"http://localhost/\">e</a>\n"\
        "<a href=\"validator.w3.org\">e</a>\n"\
        "<div class=\"center\" style=\"width:150px\" >\n"\
        "<br />\n"\
        "<code>int * func() {</code><br />\n"\
        "<code>&nbsp;&nbsp;&nbsp;return NULL;</code><br />\n"\
        "<code>}</code><br />\n"\
        "</div>\n"\
        "<p>\n"\
        "    <a href=\"http://validator.w3.org/check?uri=referer\"><img\n"\
        "      src=\"http://www.w3.org/Icons/valid-xhtml10\""\
        "   alt=\"Valid XHTML 1.0 Strict\" height=\"31\" width=\"88\" /></a>\n"\
        "</p>\n"\
        "<p>\n"\
        "    <a href=\"http://jigsaw.w3.org/css-validator/check/referer\">"\
        "    <img style=\"border:0;width:88px;height:31px\"\n"\
        "            src=\"http://jigsaw.w3.org/css-validator/images/vcss\"\n"\
        "            alt=\"Valid CSS!\" /></a>\n"\
        "</p>\n"\
        "</body>\n"\
        "\n"\
        "</html>\0";

    find_urls_input input;
    memset(&input, 0, sizeof(find_urls_input));

    input.search_name = calloc(SEARCH_NAME_LEN, sizeof(char));
    input.search_name_len = SEARCH_NAME_LEN;
    input.prefix_name = calloc(MAX_URL_LEN, sizeof(char));
    input.prefix_name_len = MAX_URL_LEN;
    input.url = calloc(MAX_URL_LEN, sizeof(char));

    strncpy(input.search_name, "example.com\0", SEARCH_NAME_LEN);
    strncpy(input.prefix_name, "http://www.example.com\0", MAX_URL_LEN);
    strncpy(input.url, "http://www.example.com/brex/index.html\0", MAX_URL_LEN);

    int matches = find_urls(html, &input);

    int i;
    for (i = 0; i < matches; i++) {
        printf("%s\n", input.ret[i]);
    }

    free_array_of_charptr_incl(&(input.ret), input.ret_len);
    free(input.search_name);
    free(input.prefix_name );
    free(input.url);
}
