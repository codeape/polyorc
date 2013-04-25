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

#include <stdlib.h>
#include <ev.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct _server_info {
    ev_io io;
    int fd;
    struct sockaddr *addr;
} server_info;

int setnonblock(int fd)
{
  int flags;

  flags = fcntl(fd, F_GETFL);
  if (-1 == flags) {
      return -1;
  }
  flags |= O_NONBLOCK;
  return fcntl(fd, F_SETFL, flags);
}

int create_server(polyarguments *arg, server_info *srv) {
    if (4 == arg->ipv) {
        orcoutcl(orc_reset, orc_blue, "Server ip version 4");
        srv->fd = socket(PF_INET, SOCK_STREAM, 0);
        if (-1 == srv->fd) {
            orcerror("Server socket error %s (%d)\n", strerror(errno), errno);
            return -1;
        }
        if (-1 == setnonblock(srv->fd)) {
            orcerror("Server socket error %s (%d)\n", strerror(errno), errno);
            return -1;
        }
        struct sockaddr_in *socket = calloc(1, sizeof(struct sockaddr_in));
        if (-1 == srv->fd) {
            orcerror("Server memmory error %s (%d)\n", strerror(errno), errno);
            return -1;
        }
        socket->sin_family = AF_INET;
        socket->sin_port = htons(arg->adminport);
        if (0 == arg->adminip) {
            socket->sin_addr.s_addr = INADDR_ANY;
            orcoutcl(orc_reset, orc_blue, "Server ip ANY");
        } else {
            int ok = inet_pton(AF_INET, arg->adminip, &(socket->sin_addr));
            if (0 == ok) {
                orcerror("Server address error for %s\n", arg->adminip);
                free(socket);
                return -1;
            } else if (0 > ok) {
                orcerror("Server address error %s (%d)\n", strerror(errno),
                         errno);
                free(socket);
                return -1;
            }
            orcoutcl(orc_reset, orc_blue, "Server ip %s", arg->adminip);
        }
        srv->addr = (struct sockaddr *) socket;
    } else if (6 == arg->ipv) {
        orcoutcl(orc_reset, orc_blue, "Server ip version 6");
        srv->fd = socket(PF_INET6, SOCK_STREAM, 0);
        if (-1 == srv->fd) {
            orcerror("Server socket error %s (%d)\n", strerror(errno), errno);
            return -1;
        }
        if (-1 == setnonblock(srv->fd)) {
            orcerror("Server socket error %s (%d)\n", strerror(errno), errno);
            return -1;
        }
        struct sockaddr_in6 *socket = calloc(1, sizeof(struct sockaddr_in6));
        if (-1 == srv->fd) {
            orcerror("Server memmory error %s (%d)\n", strerror(errno), errno);
            return -1;
        }
        socket->sin6_family = AF_INET6;
        socket->sin6_port = htons(arg->adminport);
        if (0 == arg->adminip) {
            socket->sin6_addr = in6addr_any;
            orcoutcl(orc_reset, orc_blue, "Server ip ANY");
        } else {
            int ok = inet_pton(AF_INET6, arg->adminip, &(socket->sin6_addr));
            if (0 == ok) {
                orcerror("Server address error for %s\n", arg->adminip);
                free(socket);
                return -1;
            } else if (0 > ok) {
                orcerror("Server address error %s (%d)\n", strerror(errno),
                         errno);
                free(socket);
                return -1;
            }
            orcoutcl(orc_reset, orc_blue, "Server ip %s", arg->adminip);
        }
        srv->addr = (struct sockaddr *) socket;
    } else {
        orcerror("Internal: Uknown ip version %d\n", arg->ipv);
        return -1;
    }
    orcoutcl(orc_reset, orc_blue, "Server port %d", arg->adminport);

    /* Bind socket to address */
    size_t socksize = arg->ipv == 4 ? sizeof(struct sockaddr_in) :
                                      sizeof(struct sockaddr_in6);
    if (0 != bind(srv->fd, srv->addr, socksize))
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
    server_info server;
    memset(&server, 0, sizeof(server_info));
    if(-1 == create_server(arg, &server)) {
        return;
    }

    /* Initialize and start a watcher to accepts client requests */
    ev_io_init(&(server.io), accept_cb, server.fd, EV_READ);
    ev_io_start(loop, &(server.io));

    ev_run(loop, 0);
}
