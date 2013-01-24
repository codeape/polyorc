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
#include "polyorcout.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define DEFAULT_MAX_JOBS 20
#define DEFAULT_MAX_JOBS_STR STR(DEFAULT_MAX_JOBS)

#define DEFAULT_OUT "spider.out"

const char *argp_program_version = ORC_VERSION;
const char *argp_program_bug_address = ORC_BUG_ADDRESS;

/* Program documentation. */
static char doc[] =
   "S";

/* A description of the arguments we accept. */
static char args_doc[] = "URL";

/* The options we understand. */
static struct argp_option options[] = {
    {"quiet",        'q', 0,       0, "Don't produce any output" },
    {"verbose",      'v', 0,       0, "Produce verbose output" },
    {"debug",        'd', 0,       0, "Produce debug and verbose output" },
    {"color",        'c', 0,       0, "Color output" },
    {"no-color",     'n', 0,       0, "No color output" },
    {"job",          'j', 0,       0, "Max parallell downloads (default " \
                                      DEFAULT_MAX_JOBS_STR ")" },
    {"out",          'o', "FILE",  0, "Output file (default " DEFAULT_OUT ")"},
    {"exclude",     1001, "REGEX", 0, "Exclude pattern" },
    { 0 }
};

static void check_verbosity(struct argp_state *state,
                            arguments *arg, enum polyorc_verbosity newarg)
{
    if (orcm_not_set != arg->verbosity && newarg != arg->verbosity) {
        orcerror("You can not combine quiet, verbose and debug options.\n");
        argp_usage(state);
    }
}


static void check_color(struct argp_state *state,
                        arguments *arg, enum polyorc_color newarg)
{
    if (orcc_not_set != arg->color && newarg != arg->color) {
        orcerror("You can not combine color and no-color options.\n");
        argp_usage(state);
    }
}

/* Parse a single option. */
static error_t parse_opt(int key, char *opt_arg, struct argp_state *state)
{
    /* Get the input argument from argp_parse, which we
       know is a pointer to our arguments structure. */
    arguments *arg = state->input;

    switch (key) {
    case 'q':
        check_verbosity(state, arg, orcm_quiet);
        arg->verbosity = orcm_quiet;
        break;
    case 'v':
        check_verbosity(state, arg, orcm_verbose);
        arg->verbosity = orcm_verbose;
        break;
    case 'd':
        check_verbosity(state, arg, orcm_debug);
        arg->verbosity = orcm_debug;
        break;
    case 'c':
        check_color(state,arg, orcc_use_color);
        arg->color = orcc_use_color;
        break;
    case 'n':
        check_color(state,arg, orcc_no_color);
        arg->color = orcc_no_color;
        break;
    case 'j':
        if(1 != sscanf(opt_arg, "%d", &(arg->max_jobs))) {
            orcerror("Job set to a non integer value.\n");
            argp_usage(state);
        }
        if (1 > arg->max_jobs) {
            orcerror("Job set to a 0 or a negative value.\n");
            argp_usage(state);
        }
        break;
    case 1001:
        arg->excludes_len++;
        size_t size = arg->excludes_len * sizeof(*(arg->excludes));
        char **tmp;
        tmp = realloc(arg->excludes, size);
        if (0 == tmp) {
            if (0 != arg->excludes) {
                free(arg->excludes);
            }
            orcerror("%s (%d)\n", strerror(errno), errno);
            exit(EXIT_FAILURE);
        }
        tmp[arg->excludes_len - 1] = opt_arg;
        arg->excludes = tmp;
        break;
    case 'o':
        arg->out_file = opt_arg;
        break;
    case ARGP_KEY_ARG:
        if (state->arg_num > 1) {
            /* Too many arguments. */
            argp_usage(state);
        }
        if (('h'== opt_arg[0] && 't' == opt_arg[1] &&'t' == opt_arg[2] &&
             'p' == opt_arg[3] && ':' == opt_arg[4] && '/' == opt_arg[5] &&
             '/' == opt_arg[6]) ||
            ('h'== opt_arg[0] && 't' == opt_arg[1] &&'t' == opt_arg[2] &&
             'p' == opt_arg[3] && 's' == opt_arg[4] && ':' == opt_arg[5] &&
             '/' == opt_arg[6] && '/'== opt_arg[7]))
        {
            arg->url = opt_arg;
        } else {
            orcerror("Add http:// or https:// to your target url.\n",
                     strerror(errno), errno);
            exit(EXIT_FAILURE);
        }
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
    arg.verbosity = orcm_not_set;
    arg.color = orcc_not_set;
    arg.max_jobs = DEFAULT_MAX_JOBS;
    arg.url = 0;
    arg.out_file = DEFAULT_OUT;
    arg.excludes = 0;
    arg.excludes_len = 0;

    /* Parse our arguments; every option seen by parse_opt will
       be reflected in arguments. */
    argp_parse(&argp, argc, argv, 0, 0, &arg);

    if (orcm_not_set == arg.verbosity) {
        arg.verbosity = orcm_normal;
    }

    if (orcc_not_set == arg.color) {
        arg.color = orcc_no_color;
    }

    init_polyorcout(arg.verbosity, arg.color);

    print_splash();
    crawl(&arg);

    free(arg.excludes);

    orcout(orcm_quiet, "Done!\n");
    return EXIT_SUCCESS;
}
