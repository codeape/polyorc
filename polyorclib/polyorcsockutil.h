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

#ifndef POLYORCSOCKUTIL_H
#define POLYORCSOCKUTIL_H

#include <ev.h>
#include <arpa/inet.h>

typedef union _orc_sockaddr {
    struct sockaddr_in addr4;
    struct sockaddr_in6 addr6;
} orc_sockaddr;

typedef struct _orc_socket_info {
    ev_io io;
    int fd;
    int ipv;
    int addr_size;
    orc_sockaddr addr;
} orc_socket_info;

int setnonblock(int fd);

void init_socket(const int ipv, orc_socket_info *srv);

int create_socket(const char *ip, const int port, orc_socket_info *srv);

#endif
