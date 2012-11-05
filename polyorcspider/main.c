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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <argp.h>

#include "config.h"
#include "common.h"
#include "spider.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

const char *argp_program_version = ORC_VERSION;
const char *argp_program_bug_address = ORC_BUG_ADDRESS;

/* Program documentation. */
static char doc[] =
   "S";

/* A description of the arguments we accept. */
static char args_doc[] = "URL";

/* The options we understand. */
static struct argp_option options[] = {
    {"verbose",      'v', 0,       0, "Produce verbose output" },
    {"quiet",        'q', 0,       0, "Don't produce any output" },
    {"exclude",     1001, "REGEX", 0, "Exclude pattern" },
    { 0 }
};

/* Parse a single option. */
static error_t parse_opt(int key, char *opt_arg, struct argp_state *state)
{
    /* Get the input argument from argp_parse, which we
       know is a pointer to our arguments structure. */
    arguments *arg = state->input;

    switch (key) {
    case 'q':
        arg->silent = 1;
        break;
    case 'v':
        arg->verbose = 1;
        break;
    case 1001:
        arg->excludes_len++;
        size_t size = arg->excludes_len * sizeof(*(arg->excludes));
        char **tmp;
        tmp = realloc(arg->excludes, size);
        if (0 == tmp) {
            fprintf(stderr, "ERROR: %s (%d)\n", strerror(errno), errno);
            exit(EXIT_FAILURE);
        }
        tmp[arg->excludes_len - 1] = opt_arg;
        arg->excludes = tmp;
        break;
    case ARGP_KEY_ARG:
        if (state->arg_num > 1) {
            /* Too many arguments. */
            argp_usage(state);
        }
        arg->url = opt_arg;
        break;
    case ARGP_KEY_END:
        if (state->arg_num < 1) {
            /* Not enough arguments. */
            argp_usage(state);
        }
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc };

// url
// connections


int main(int argc, char *argv[])
{
    arguments arg;

    /* Default values. */
    arg.silent = 0;
    arg.verbose = 0;
    arg.url = 0;
    arg.excludes = 0;
    arg.excludes_len = 0;

    /* Parse our arguments; every option seen by parse_opt will
       be reflected in arguments. */
    argp_parse(&argp, argc, argv, 0, 0, &arg);

    crawl(&arg);

    free(arg.excludes);
    arg.excludes = 0;

    return EXIT_SUCCESS;
}
