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

#include <unistd.h>

#include "common.h"
#include "client.h"
#include "polyorcsockutil.h"

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


void client_loop(bossarguments *arg) {
    orc_socket_info client;
    init_socket(arg->ipv, &client);
    if(-1 == create_client(arg, &client)) {
        return;
    }
}
