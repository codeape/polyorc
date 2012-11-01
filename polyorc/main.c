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

#include <stdlib.h>
#include <argp.h>
#include <stdio.h>

#include <polyorcutils.h>
#include "config.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define ORC_DEFAULT_ADMIN_PORT 7711
#define ORC_DEFAULT_ADMIN_PORT_STR STR(ORC_DEFAULT_ADMIN_PORT)

const char *argp_program_version = ORC_VERSION;
const char *argp_program_bug_address = ORC_BUG_ADDRESS;

/* Program documentation. */
static char doc[] =
   "S";

/* A description of the arguments we accept. */
static char args_doc[] = "PASSWORD";

/* The options we understand. */
static struct argp_option options[] = {
    {"verbose",      'v', 0,      0, "Produce verbose output" },
    {"quiet",        'q', 0,      0, "Don't produce any output" },
    {"admin-port",   'a', "PORT", 0, "The admin port (default " \
                                     ORC_DEFAULT_ADMIN_PORT_STR  ")"},
    {"admin-ip",     'i', "IP",   0, "Bind admin port to spesifc ip."},
    { 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments {
    char *password;
    int silent, verbose, adminport;
};

/* Parse a single option. */
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    /* Get the input argument from argp_parse, which we
       know is a pointer to our arguments structure. */
    struct arguments *arguments = state->input;

    switch (key) {
    case 'q':
        arguments->silent = 1;
        break;
    case 'v':
        arguments->verbose = 1;
        break;
    case 'a':
        if(1 != sscanf(arg, "%d", &(arguments->adminport))) {
            fprintf(stderr, "Admin port set to a non integer value.\n");
            argp_usage(state);
        }
        if (arguments->adminport < 1 ||
            arguments->adminport > (intpow(2, 16) - 1))
        {
            fprintf(stderr,
                    "Admin port set to a value outside range %d to %d.\n",
                    1,
                    (intpow(2,16) - 1));
            argp_usage(state);
        }
        break;
    case ARGP_KEY_ARG:
        if (state->arg_num > 1) {
            /* Too many arguments. */
            argp_usage(state);
        }
        arguments->password = arg;
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

int
main(int argc, char *argv[]) {
    struct arguments arguments;

    /* Default values. */
    arguments.silent = 0;
    arguments.verbose = 0;
    arguments.adminport = ORC_DEFAULT_ADMIN_PORT;

    /* Parse our arguments; every option seen by parse_opt will
       be reflected in arguments. */
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    printf("PASSWORD = %s\nADMINPORT = %d\n"
           "VERBOSE = %s\nSILENT = %s\n",
           arguments.password,
           arguments.adminport,
           arguments.verbose ? "yes" : "no",
           arguments.silent ? "yes" : "no");

    return EXIT_SUCCESS;
}
