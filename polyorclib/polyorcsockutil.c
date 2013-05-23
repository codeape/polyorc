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

#include "polyorcsockutil.h"
#include "polyorcout.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

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

void init_socket(const int ipv, orc_socket_info *srv) {
    memset(srv, 0, sizeof(orc_socket_info));
    if (4 == ipv) {
        srv->ipv = 4;
        srv->addr_size = sizeof(struct sockaddr_in);
    } else if (6 == ipv) {
        srv->ipv = 6;
        srv->addr_size = srv->addr_size = sizeof(struct sockaddr_in6);
    }
}

int create_socket(const char *ip, const int port, orc_socket_info *srv,
                  int block)
{
    orcoutcl(orc_reset, orc_blue, "Socket ip version %d", srv->ipv);
    if (4 == srv->ipv) {
        srv->fd = socket(PF_INET, SOCK_STREAM, 0);
        if (-1 == srv->fd) {
            orcerror("Socket error %s (%d)\n", strerror(errno), errno);
            return -1;
        }
        if (0 == block) {
            if (-1 == setnonblock(srv->fd)) {
                orcerror("Socket nonblock error %s (%d)\n", strerror(errno), errno);
                return -1;
            }
        }
        srv->addr.addr4.sin_family = AF_INET;
        srv->addr.addr4.sin_port = htons(port);
        if (0 == ip) {
            srv->addr.addr4.sin_addr.s_addr = INADDR_ANY;
            orcoutcl(orc_reset, orc_blue, "Socket ip ANY");
        } else {
            int ok = inet_pton(AF_INET, ip, &(srv->addr.addr4.sin_addr));
            if (0 == ok) {
                orcerror("Socket address error for %s\n", ip);
                return -1;
            } else if (0 > ok) {
                orcerror("Socket address error %s (%d)\n", strerror(errno),
                         errno);
                return -1;
            }
            orcoutcl(orc_reset, orc_blue, "Socket ip %s", ip);
        }
    } else if (6 == srv->ipv) {
        srv->fd = socket(PF_INET6, SOCK_STREAM, 0);
        if (-1 == srv->fd) {
            orcerror("Socket error %s (%d)\n", strerror(errno), errno);
            return -1;
        }
        if (0 == block) {
            if (-1 == setnonblock(srv->fd)) {
                orcerror("Socket nonblock error %s (%d)\n", strerror(errno), errno);
                return -1;
            }
        }
        srv->addr.addr6.sin6_family = AF_INET6;
        srv->addr.addr6.sin6_port = htons(port);
        if (0 == ip) {
            srv->addr.addr6.sin6_addr = in6addr_any;
            orcoutcl(orc_reset, orc_blue, "Socket ip ANY");
        } else {
            int ok = inet_pton(AF_INET6, ip, &(srv->addr.addr6.sin6_addr));
            if (0 == ok) {
                orcerror("Socket address error for %s\n", ip);
                free(socket);
                return -1;
            } else if (0 > ok) {
                orcerror("Socket address error %s (%d)\n", strerror(errno),
                         errno);
                free(socket);
                return -1;
            }
            orcoutcl(orc_reset, orc_blue, "Socket ip %s", ip);
        }
    } else {
        orcerror("Uknown socket ip version %d\n", srv->ipv);
        return -1;
    }
    orcoutcl(orc_reset, orc_blue, "Socket port %d", port);
    return 0;
}
