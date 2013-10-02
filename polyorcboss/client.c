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

#include <errno.h>
#include <string.h>
#include <ev.h>

#include <curses.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
//#include <limits.h> // MAX_INPUT

#include "common.h"
#include "client.h"
#include "polyorcsockutil.h"

//static char line_buffer[MAX_INPUT];
static int line_index;

int create_client(bossarguments *arg, orc_socket_info *clientsoc) {
    /* TODO: Make nonblocking */
    if (-1 == create_socket(arg->adminip, arg->adminport, clientsoc, 1)) {
        return -1;
    }

    /* Bind socket to address */
    struct sockaddr *addr = 4 == clientsoc->ipv ?
        (struct sockaddr *) &(clientsoc->addr.addr4) :
        (struct sockaddr *) &(clientsoc->addr.addr6);
    if (0 != connect(clientsoc->fd, addr, clientsoc->addr_size))
    {
        orcstatus(orcm_normal, orc_red, "fail", "Connect\n");
        orcerror("%s (%d)\n", strerror(errno), errno);
        return  -1;
    }
    orcstatus(orcm_normal, orc_green, "ok", "Connect\n");

    /* TODO: Remove -->*/
    const char *msg = "Test!\0";
    if (-1 == write(clientsoc->fd, msg, strlen(msg))) {
        orcstatus(orcm_normal, orc_red, "fail", "Send\n");
        orcerror("%s (%d)\n", strerror(errno), errno);
    }
    orcstatus(orcm_normal, orc_green, "ok", "Send\n");
    while (1) {

    }
    close(clientsoc->fd);
    /* TODO: <-- Remove */

    return 0;
}

static void finish(int sig)
{
    endwin();

    /* do your non-curses wrapup here */

    exit(0);
}


void curses_cb(struct ev_loop *loop, struct ev_io *event_io, int revents) {
    int ch = getch();

    if (ch != ERR) {
        /*
            Not needed!
            addch(ch); //echo on
        */
    }
}

void init_curses(struct ev_loop *loop, ev_io *io) {
    /* initialize your non-curses data structures here */
    line_index = 0;

    signal(SIGINT, finish);      /* arrange interrupts to terminate */

    initscr();      /* initialize the curses library */
    keypad(stdscr, TRUE);  /* enable keyboard mapping */
    nonl();         /* tell curses not to do NL->CR/NL on output */
    cbreak();       /* take input chars one at a time, no wait for \n */
    notimeout(stdscr, TRUE);
    //nodelay(stdscr, TRUE);
    echo();         /* echo input - in color */

    /* Initialize and start a watcher to accepts client requests */
    ev_io_init(io, curses_cb, fileno(stdin), EV_READ);
    ev_io_start(loop, io);
}

void client_loop(bossarguments *arg) {
    struct ev_loop *loop = ev_default_loop(0);
    ev_io io;
    init_curses(loop, &(io));

    /*orc_socket_info client;
    init_socket(arg->ipv, &client);
    if(-1 == create_client(arg, &client)) {
        return;
    }*/

    ev_run(loop, 0);
}
