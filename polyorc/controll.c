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

#include "controll.h"
#include "common.h"
#include "polyorcout.h"
#include "polyorcsockutil.h"

#include <stdlib.h>
#include <ev.h>
#include <errno.h>
#include <string.h>

int create_server(polyarguments *arg, orc_socket_info *srv) {
    if (-1 == create_socket(arg->adminip, arg->adminport, srv)) {
        return -1;
    }

    /* Bind socket to address */
    struct sockaddr *addr = 4 == srv->ipv ?
        (struct sockaddr *) &(srv->addr.addr4) :
        (struct sockaddr *) &(srv->addr.addr6);
    if (0 != bind(srv->fd, addr, srv->addr_size))
    {
        orcstatus(orcm_normal, orc_red, "fail", "Bind\n");
        orcerror("%s (%d)\n", strerror(errno), errno);
        return  -1;
    }
    orcstatus(orcm_normal, orc_green, "ok", "Bind\n");

    /* Start listing on the socket */
    if (0 > listen(srv->fd, 5))
    {
        orcstatus(orcm_normal, orc_green, "fail", "Listen\n");
        orcerror("%s (%d)\n", strerror(errno), errno);
        return -1;
    }
    orcstatus(orcm_normal, orc_green, "ok", "Listen\n");

    return 0;
}

void accept_cb(EV_P_ struct ev_io *event_io, int revents) {
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    client_fd = accept(event_io->fd, (struct sockaddr *)&client_addr, &client_len);
    if (-1 != client_fd) {
        orcstatus(orcm_normal, orc_green, "ok", "fd = %d\n", client_fd);
    } else {
        orcerror("%s (%d)\n", strerror(errno), errno);
        exit(EXIT_FAILURE);
    }
}

void controll_loop(polyarguments *arg) {
    /* use ev_loop_new for the event loops in  all worker threads */
    struct ev_loop *loop = ev_default_loop(0);
    orc_socket_info server;
    init_socket(arg->ipv, &server);
    if(-1 == create_server(arg, &server)) {
        return;
    }

    /* Initialize and start a watcher to accepts client requests */
    ev_io_init(&(server.io), accept_cb, server.fd, EV_READ);
    ev_io_start(loop, &(server.io));

    ev_run(loop, 0);
}
