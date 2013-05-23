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
#include "polyorcbintree.h"

#include <stdlib.h>
#include <ev.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

typedef struct _client_data {
    int fd;
    ev_io io;
    size_t buff_len;
    char *buff;
    char *buff_ptr;
} client_data;

bintree_root clients;

int create_server(polyarguments *arg, orc_socket_info *srv) {
    if (-1 == create_socket(arg->adminip, arg->adminport, srv, 0)) {
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

void client_cb(struct ev_loop *loop, struct ev_io *event_io, int revents) {
    //ev_io_stop(loop, event_io);
    client_data* client = bintree_find(&clients, &(event_io->fd));
    if (0 == client) {
        orcerror("Missing client!\n");
        exit(EXIT_FAILURE);
    }
    if (0 == client->buff) {
        client->buff_len = 10;
        client->buff = calloc(client->buff_len, sizeof(char));
        if (0 == client->buff) {
            orcerror("%s (%d)\n", strerror(errno), errno);
            exit(EXIT_FAILURE);
        }
        client->buff_ptr = client->buff;
    }
    size_t curr = client->buff_ptr - client->buff;
    ssize_t len = read(client->fd, client->buff_ptr, client->buff_len - curr);
    if (0 < len) {
        printf("%s", client->buff_ptr); /* remove */
        client->buff_ptr = client->buff_ptr + len;
        int pos = client->buff_ptr - client->buff == client->buff_len;
        if (pos == client->buff_len) {
            client->buff_len += 100;
            client->buff = realloc(client->buff, client->buff_len * sizeof(char));
            if (0 == client->buff) {
                orcerror("%s (%d)\n", strerror(errno), errno);
                exit(EXIT_FAILURE);
            }
            client->buff_ptr = client->buff + pos;
        }
    } else if (0 > len) {
        orcerror("%s (%d)\n", strerror(errno), errno);
        exit(EXIT_FAILURE);
    } else {
        client->buff_ptr = client->buff_ptr + len;
        client->buff_ptr[0] = '\0';
        client->buff_ptr = client->buff;
        orcstatus(orcm_normal, orc_green, "close read", "%s\n", client->buff);
        ev_io_stop(loop, event_io);
        bintree_delete(&(clients), &(client->fd));
    }
}

void accept_cb(struct ev_loop *loop, ev_io *event_io, int revents) {
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    client_fd = accept(event_io->fd, (struct sockaddr *)&client_addr, &client_len);
    if (-1 != client_fd) {
        orcstatus(orcm_normal, orc_green, "ok", "fd = %d\n", client_fd);
        client_data* client = calloc(1, sizeof(client_data));
        client->fd = client_fd;
        ev_init(&(client->io), client_cb);
        ev_io_set(&(client->io), client->fd, EV_READ);
        ev_io_start (loop, &(client->io));
        bintree_add(&clients, &(client->fd), client);
    } else {
        orcerror("%s (%d)\n", strerror(errno), errno);
        exit(EXIT_FAILURE);
    }
}

void free_tree(void **key, void **value, const enum free_cmd cmd) {
    /* free not needed key points to value->fd */
    (*key) = 0;
    client_data* client = (*value);
    if (0 != client->buff) {
        free(client->buff);
    }
    free(*value);
    (*value) = 0;
}

void controll_loop(polyarguments *arg) {
    bintree_init(&clients, bintree_inteq, free_tree);

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
