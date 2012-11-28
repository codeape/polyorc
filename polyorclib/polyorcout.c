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

#include "polyorcout.h"

#include <stdio.h>
#include <stdarg.h>

#define RESETCOLOR "\033[0m"
#define SELECTCOLOR "\033[%d;%d;m"

static enum polyorc_verbosity orc_verbosity = orcm_normal;
static enum polyorc_color orc_color = orcc_no_color;

/**
 * Sets the output verbosity level and if there will be colored
 * output.
 *
 * @author Oscar Norlander
 *
 * @param v Verbosity level
 * @param c Color mode
 */
void init_polyorcout(enum polyorc_verbosity v, enum polyorc_color c) {
    orc_verbosity = v;
    orc_color = c;
}

/**
 * Returns the output level
 *
 * @author Oscar Norlander
 *
 * @return enum polyorc_verbosity
 */
enum polyorc_verbosity get_verbosity() {
    return orc_verbosity;
}

/**
 * Returns the color mode
 *
 * @author Oscar Norlander
 *
 * @return enum polyorc_color
 */
enum polyorc_color get_color() {
    return orc_color;
}

/**
 * Outputs a error message on stderr
 *
 * @author Oscar Norlander
 *
 * @param format The same as for printf
 */
void orcerror(const char *format, ...) {
    va_list argptr;
    printf("ERROR: ");
    va_start(argptr, format);
    vprintf(format, argptr);
    va_end(argptr);
}

/**
 * Prints a message if the verbosity level of the printout is
 * equal or less than the level set with init_polyorcout
 *
 * @author Oscar Norlander
 *
 * @param verbosity Level
 * @param format The same as for printf
 */
void orcout(enum polyorc_verbosity verbosity, const char *format, ...) {
    if (verbosity <= orc_verbosity) {
        va_list argptr;
        va_start(argptr, format);
        vprintf(format, argptr);
        va_end(argptr);
    }
}

/**
 * Prints foreground colored output and reset the color
 *
 * @author Oscar Norlander
 *
 * @param attr Color attribute
 * @param cv Color
 * @param format The same as for printf
 */
void orcoutc(enum polyorc_color_attr attr, enum polyorc_color_val cv,
             const char *format, ...) {
    if (orcc_use_color ==  orc_color) {
        printf(SELECTCOLOR, attr, cv);
    }
    va_list argptr;
    va_start(argptr, format);
    vprintf(format, argptr);
    va_end(argptr);
    if (orcc_use_color ==  orc_color) {
        printf(RESETCOLOR);
    }
}

/**
 * Prints foreground colored output and inserts a new line
 * character after reseting the color
 *
 * @author Oscar Norlander
 *
 * @param attr Color attribute
 * @param cv Color
 * @param format The same as for printf
 */
void orcoutcl(enum polyorc_color_attr attr, enum polyorc_color_val cv,
              const char *format, ...) {
    if (orcc_use_color ==  orc_color) {
        printf(SELECTCOLOR, attr, cv);
    }
    va_list argptr;
    va_start(argptr, format);
    vprintf(format, argptr);
    va_end(argptr);
    if (orcc_use_color ==  orc_color) {
        printf(RESETCOLOR);
    }
    printf("\n");
}

/**
 * Prints a colored (if enabled) message and a formated message.
 * If color is enabled a color (associated with the status code
 * ) is put on the status message
 *
 * @author Oscar Norlander
 *
 * @param verbosity Level
 * @param s Status code
 * @param msg A status message
 * @param format The same as for printf
 */
void orcstatus(enum polyorc_verbosity verbosity, enum polyorc_color_val color,
            const char *msg, const char *format, ...) {
    if (verbosity <= orc_verbosity) {
        printf("[ ");
        orcoutc(orc_reset, color, msg);
        printf(" ] ");
        va_list argptr;
        va_start(argptr, format);
        vprintf(format, argptr);
        va_end(argptr);
    }
}

/**
 * Something nice to look at
 *
 * @author Oscar Norlander
 */
void print_splash() {
    orcoutcl(orc_reset, orc_red,
        "...................................................................");
    orcoutcl(orc_reset, orc_green,
        "@@@@@@@    @@@@@@   @@@       @@@ @@@   @@@@@@   @@@@@@@    @@@@@@@");
    orcoutcl(orc_reset, orc_green,
        "@@@@@@@@  @@@@@@@@  @@@       @@@ @@@  @@@@@@@@  @@@@@@@@  @@@@@@@@");
    orcoutcl(orc_reset, orc_green,
        "@@!  @@@  @@!  @@@  @@!       @@! !@@  @@!  @@@  @@!  @@@  !@@");
    orcoutcl(orc_reset, orc_green,
        "!@!  @!@  !@!  @!@  !@!       !@! @!!  !@!  @!@  !@!  @!@  !@!");
    orcoutcl(orc_reset, orc_green,
        "@!@@!@!   @!@  !@!  @!!        !@!@!   @!@  !@!  @!@!!@!   !@!");
    orcoutcl(orc_bright, orc_green,
        "!!@!!!    !@!  !!!  !!!         @!!!   !@!  !!!  !!@!@!    !!!");
    orcoutcl(orc_bright, orc_green,
        "!!:       !!:  !!!  !!:         !!:    !!:  !!!  !!: :!!   :!!");
    orcoutcl(orc_bright, orc_green,
        ":!:       :!:  !:!   :!:        :!:    :!:  !:!  :!:  !:!  :!:");
    orcoutcl(orc_bright, orc_green,
        " ::       ::::: ::   :: ::::     ::    ::::: ::  ::   :::   ::: :::");
    orcoutcl(orc_bright, orc_green,
        " :         : :  :   : :: : :     :      : :  :    :   : :   :: :: :");
    orcoutcl(orc_reset, orc_red,
        "...................................................................");
}
