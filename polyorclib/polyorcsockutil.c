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
#include <arpa/inet.h>
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

int create_socket(const char *adminip,
                  const int adminport,
                  const int ipv,
                  orc_socket_info *srv)
{
    if (4 == ipv) {
        orcoutcl(orc_reset, orc_blue, "Polyorc socket ip version 4");
        srv->fd = socket(PF_INET, SOCK_STREAM, 0);
        if (-1 == srv->fd) {
            orcerror("Polyorc socket error %s (%d)\n", strerror(errno), errno);
            return -1;
        }
        if (-1 == setnonblock(srv->fd)) {
            orcerror("Polyorc socket error %s (%d)\n", strerror(errno), errno);
            return -1;
        }
        struct sockaddr_in *socket = calloc(1, sizeof(struct sockaddr_in));
        if (-1 == srv->fd) {
            orcerror("Polyorc socket memmory error %s (%d)\n",
                     strerror(errno), errno);
            return -1;
        }
        socket->sin_family = AF_INET;
        socket->sin_port = htons(adminport);
        if (0 == adminip) {
            socket->sin_addr.s_addr = INADDR_ANY;
            orcoutcl(orc_reset, orc_blue, "Polyorc socket ip ANY");
        } else {
            int ok = inet_pton(AF_INET, adminip, &(socket->sin_addr));
            if (0 == ok) {
                orcerror("Polyorc socket address error for %s\n", adminip);
                free(socket);
                return -1;
            } else if (0 > ok) {
                orcerror("Polyorc socket address error %s (%d)\n", strerror(errno),
                         errno);
                free(socket);
                return -1;
            }
            orcoutcl(orc_reset, orc_blue, "Polyorc socket ip %s", adminip);
        }
        srv->addr = (struct sockaddr *) socket;
    } else if (6 == ipv) {
        orcoutcl(orc_reset, orc_blue, "Polyorc socket ip version 6");
        srv->fd = socket(PF_INET6, SOCK_STREAM, 0);
        if (-1 == srv->fd) {
            orcerror("Polyorc socket error %s (%d)\n", strerror(errno), errno);
            return -1;
        }
        if (-1 == setnonblock(srv->fd)) {
            orcerror("Polyorc socket error %s (%d)\n", strerror(errno), errno);
            return -1;
        }
        struct sockaddr_in6 *socket = calloc(1, sizeof(struct sockaddr_in6));
        if (-1 == srv->fd) {
            orcerror("Polyorc socket memmory error %s (%d)\n",
                     strerror(errno), errno);
            return -1;
        }
        socket->sin6_family = AF_INET6;
        socket->sin6_port = htons(adminport);
        if (0 == adminip) {
            socket->sin6_addr = in6addr_any;
            orcoutcl(orc_reset, orc_blue, "Polyorc socket ip ANY");
        } else {
            int ok = inet_pton(AF_INET6, adminip, &(socket->sin6_addr));
            if (0 == ok) {
                orcerror("Polyorc socket address error for %s\n", adminip);
                free(socket);
                return -1;
            } else if (0 > ok) {
                orcerror("Polyorc socket address error %s (%d)\n", strerror(errno),
                         errno);
                free(socket);
                return -1;
            }
            orcoutcl(orc_reset, orc_blue, "Polyorc socket ip %s", adminip);
        }
        srv->addr = (struct sockaddr *) socket;
    } else {
        orcerror("Internal: Uknown socket ip version %d\n", ipv);
        return -1;
    }
    orcoutcl(orc_reset, orc_blue, "Polyorc socket port %d", adminport);
    return 0;
}
