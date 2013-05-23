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

#include <argp.h>
#include <stdlib.h>

#include "common.h"
#include "polyorcutils.h"
#include "client.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define ORC_DEFAULT_ADMIN_PORT 7711
#define ORC_DEFAULT_ADMIN_PORT_STR STR(ORC_DEFAULT_ADMIN_PORT)

#define ORC_DEFAULT_ADMIN_IP "127.0.0.1"

const char *argp_program_version = ORC_VERSION;
const char *argp_program_bug_address = ORC_BUG_ADDRESS;

/* Program documentation. */
static char doc[] =
   "This is a http load generator.";

/* A description of the arguments we accept. */
static char args_doc[] = "URL";

/* The options we understand. */
static struct argp_option options[] = {
    {"quiet",        'q', 0,       0, "Don't produce any output" },
    {"verbose",      'v', 0,       0, "Produce verbose output" },
    {"debug",        'd', 0,       0, "Produce debug and verbose output" },
    {"color",        'c', 0,       0, "Color output" },
    {"no-color",     'n', 0,       0, "No color output" },
    {"admin-port",   'a', "PORT",  0, "The admin port (default " \
                                      ORC_DEFAULT_ADMIN_PORT_STR  ")"},
    {"admin-ip",     'i', "IP",    0, "Admin ip (default " \
                                      ORC_DEFAULT_ADMIN_IP ")"},
    {"ipv4",         '4', 0,       0, "Use ipv 4 for admin socket (default)"},
    {"ipv6",         '6', 0,       0, "Use ipv 6 for admin socket"},
    { 0 }
};

static void check_verbosity(struct argp_state *state,
                            bossarguments *arg, enum polyorc_verbosity newarg)
{
    if (orcm_not_set != arg->verbosity && newarg != arg->verbosity) {
        orcerror("You can not combine quiet, verbose and debug options.\n");
        argp_usage(state);
    }
}


static void check_color(struct argp_state *state,
                        bossarguments *arg, enum polyorc_color newarg)
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
    bossarguments *arg = state->input;

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
    case 'a':
        if(1 != sscanf(opt_arg, "%d", &(arg->adminport))) {
            fprintf(stderr, "Admin port set to a non integer value.\n");
            argp_usage(state);
        }
        if (arg->adminport < 1 ||
            arg->adminport > (intpow(2, 16) - 1))
        {
            fprintf(stderr,
                    "Admin port set to a value outside range %d to %d.\n",
                    1,
                    (intpow(2,16) - 1));
            argp_usage(state);
        }
        break;
    case 'n':
        check_color(state,arg, orcc_no_color);
        arg->color = orcc_no_color;
        break;
    case 'i':
        arg->adminip = opt_arg;
        break;
    case '4':
        arg->ipv = 4;
        break;
    case '6':
        arg->ipv = 6;
        break;
    case ARGP_KEY_ARG:
    case ARGP_KEY_END:
        if (state->arg_num != 0) {
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

int main(int argc, char *argv[])
{
    bossarguments arg;

    /* Default values. */
    arg.verbosity = orcm_not_set;
    arg.color = orcc_not_set;
    arg.url = 0;
    arg.ipv = 4;
    arg.adminip = ORC_DEFAULT_ADMIN_IP;
    arg.adminport = ORC_DEFAULT_ADMIN_PORT;

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

    client_loop(&arg);

    orcout(orcm_quiet, "Done!\n");
    return EXIT_SUCCESS;
}
