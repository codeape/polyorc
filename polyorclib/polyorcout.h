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

#ifndef POLYORCOUT_H
#define POLYORCOUT_H

enum polyorc_verbosity {
    orcm_not_set = 0,
    orcm_quiet,
    orcm_normal,
    orcm_verbose,
    orcm_debug
};

enum polyorc_color {
    orcc_not_set  = 0,
    orcc_use_color,
    orcc_no_color
};

enum polyorc_color_attr {
    orc_reset = 0,
    orc_bright = 1,
    orc_dim = 2,
    orc_underline = 3,
    orc_blink = 5,
    orc_reverse = 7,
    orc_hidden = 8
};


enum polyorc_color_val {
    orc_black = 30,
    orc_red = 31,
    orc_green = 32,
    orc_yellow = 33,
    orc_blue = 34,
    orc_magenta = 35,
    orc_cyan = 36,
    orc_white =37
};

void init_polyorcout(enum polyorc_verbosity, enum polyorc_color);

void orcerror(const char* format, ...);

void orcerrno(int);

enum polyorc_verbosity get_verbosity();

enum polyorc_color get_color();

void orcout(enum polyorc_verbosity, const char*, ...);

void orcoutc(enum polyorc_color_attr, enum polyorc_color_val, const char*, ...);

void orcoutcl(enum polyorc_color_attr, enum polyorc_color_val,
              const char* format, ...);

void orcstatus(enum polyorc_verbosity, enum polyorc_color_val, const char *,
            const char*, ...);

void print_splash();

#endif
