#include "spider.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <ev.h>

/*
    char **urls = 0;
    int urls_len = 0;
    if(-1 == (urls_len = find_urls(chunk.memory, arg.excludes, 
                       arg.excludes_len, &urls_len, &urls)))
    {
        if (0 != urls_len) {
            free_array_of_charptr_incl(&urls, urls_len);
        }
        return EXIT_FAILURE;
    }

    int i;
    for (i = 0; i < urls_len; i++) {
        printf("%s\n", urls[i]);
    }

    free_array_of_charptr_incl(&urls, urls_len);
*/

/* Global information, common to all connections */
struct SpiderGlobalInfo {
    struct ev_loop *loop;
    struct ev_timer timer_event;
    int still_running;
    CURLM *multi;
};

/* Information associated with a specific easy handle */
struct ConnInfo {
    CURL *easy;
    char *url;
    struct SpiderGlobalInfo *global;
    char error[CURL_ERROR_SIZE];
};

/* Information associated with a specific socket */
struct SockInfo {
    curl_socket_t sockfd;
    CURL *easy;
    int action;
    long timeout;
    struct ev_io ev;
    int evset;
    struct SpiderGlobalInfo *global;
};

/* Die if we get a bad CURLMcode somewhere */
static void mcode_or_die(const char *where, CURLMcode code) {
    if (CURLM_OK != code) {
        const char *s;
        switch (code) {
        case CURLM_CALL_MULTI_PERFORM: s = "CURLM_CALL_MULTI_PERFORM"; break;
        case CURLM_BAD_HANDLE:         s = "CURLM_BAD_HANDLE";         break;
        case CURLM_BAD_EASY_HANDLE:    s = "CURLM_BAD_EASY_HANDLE";    break;
        case CURLM_OUT_OF_MEMORY:      s = "CURLM_OUT_OF_MEMORY";      break;
        case CURLM_INTERNAL_ERROR:     s = "CURLM_INTERNAL_ERROR";     break;
        case CURLM_UNKNOWN_OPTION:     s = "CURLM_UNKNOWN_OPTION";     break;
        case CURLM_LAST:               s = "CURLM_LAST";               break;
        default: s = "CURLM_unknown";
            break;
        case CURLM_BAD_SOCKET:         s = "CURLM_BAD_SOCKET";
            printf("ERROR: %s returns %s\n", where, s);
            /* ignore this error */
            return;
        }
        printf("ERROR: %s returns %s\n", where, s);
        exit(code);
    }
}

/* Check for completed transfers, and remove their easy handles */
static void check_multi_info(struct SpiderGlobalInfo *g) {
    char *eff_url;
    CURLMsg *msg;
    int msgs_left;
    struct ConnInfo *conn;
    CURL *easy;
    CURLcode res;

    printf("REMAINING: %d\n", g->still_running);
    while ((msg = curl_multi_info_read(g->multi, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            easy = msg->easy_handle;
            res = msg->data.result;
            curl_easy_getinfo(easy, CURLINFO_PRIVATE, &conn);
            curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &eff_url);
            printf("DONE: %s => (%d) %s\n", eff_url, res, conn->error);
            curl_multi_remove_handle(g->multi, easy);
            free(conn->url);
            // create new readers here?
            // free read memmory here
            curl_easy_cleanup(easy);
            free(conn);
        }
    }
}

/* Called by libevent when our "wait for socket actions" timeout expires */
static void socket_action_timer_cb(EV_P_ struct ev_timer *timer, int revents) {
    struct SpiderGlobalInfo *g = (struct SpiderGlobalInfo *)timer->data;
    CURLMcode rc;

    /* Do the timeout action */
    rc = curl_multi_socket_action(g->multi, CURL_SOCKET_TIMEOUT, 0,
                                  &g->still_running);
    mcode_or_die("socket_action_timer_cb: curl_multi_socket_action", rc);
    check_multi_info(g);
}

/* Update the event timer ("wait for socket actions") after curl_multi library
   calls */
static int multi_timer_cb(CURLM *multi, long timeout_ms,
                          struct SpiderGlobalInfo *g) {
    ev_timer_stop(g->loop, &g->timer_event);
    if (timeout_ms > 0) {
        double  t = timeout_ms / 1000;
        ev_timer_init(&g->timer_event, socket_action_timer_cb, t, 0.);
        ev_timer_start(g->loop, &g->timer_event);
    } else {
        socket_action_timer_cb(g->loop, &g->timer_event, 0);
    }
    return 0;
}

/* Called by libevent when we get action on a multi socket */
static void event_cb(EV_P_ struct ev_io *w, int revents) {
    struct SpiderGlobalInfo *g = (struct SpiderGlobalInfo *)w->data;
    CURLMcode rc;

    int action = (revents & EV_READ ? CURL_POLL_IN : 0) |
          (revents & EV_WRITE ? CURL_POLL_OUT : 0);
    rc = curl_multi_socket_action(g->multi, w->fd, action, &g->still_running);
    mcode_or_die("event_cb: curl_multi_socket_action", rc);
    check_multi_info(g);
    if (g->still_running <= 0) {
        printf("last transfer done, kill timeout\n");
        ev_timer_stop(g->loop, &g->timer_event);
    }
}

/* Clean up the SockInfo structure */
static void remsock(struct SockInfo *f, struct SpiderGlobalInfo *g) {
    if (f) {
        if (f->evset) {
            ev_io_stop(g->loop, &f->ev);
        }
        free(f);
    }
}

/* Assign information to a SockInfo structure */
static void setsock(struct SockInfo *f, curl_socket_t s, CURL *e, int act,
                    struct SpiderGlobalInfo *g) {
    int kind = (act & CURL_POLL_IN ? EV_READ : 0) |
          (act & CURL_POLL_OUT ? EV_WRITE : 0);

    f->sockfd = s;
    f->action = act;
    f->easy = e;
    if (f->evset) {
        ev_io_stop(g->loop, &f->ev);
    }
    ev_io_init(&f->ev, event_cb, f->sockfd, kind);
    f->ev.data = g;
    f->evset = 1;
    ev_io_start(g->loop, &f->ev);
}

/* Initialize a new SockInfo structure */
static void addsock(curl_socket_t s, CURL *easy, int action,
                    struct SpiderGlobalInfo *g) {
    struct SockInfo *fdp = calloc(sizeof(struct SockInfo), 1);

    fdp->global = g;
    setsock(fdp, s, easy, action, g);
    curl_multi_assign(g->multi, s, fdp);
}

/* Notifies about updates on a socket file descriptor */
static int sock_cb(CURL *e, curl_socket_t s, int what, void *cbp, void *sockp) {
    struct SpiderGlobalInfo *g = (struct SpiderGlobalInfo *)cbp;
    struct SockInfo *fdp = (struct SockInfo *)sockp;
    const char *whatstr[] = { "none", "IN", "OUT", "INOUT", "REMOVE" };

    printf("socket callback: s=%d e=%p what=%s ", s, e, whatstr[what]);
    if (what == CURL_POLL_REMOVE) {
        printf("\n");
        remsock(fdp, g);
    } else {
        if (!fdp) {
            printf("Adding data: %s\n", whatstr[what]);
            addsock(s, e, what, g);
        } else {
            printf("Changing action from %s to %s\n",
                   whatstr[fdp->action], whatstr[what]);
            setsock(fdp, s, e, what, g);
        }
    }
    return 0;
}

/* CURLOPT_WRITEFUNCTION */
static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *data) {
    size_t realsize = size * nmemb;
    struct ConnInfo *conn = (struct ConnInfo *)data;
    (void)ptr;
    (void)conn;
    return realsize;
}


/* CURLOPT_PROGRESSFUNCTION */
static int prog_cb(void *p, double dltotal, double dlnow, double ult,
                   double uln) {
    struct ConnInfo *conn = (struct ConnInfo *)p;
    (void)ult;
    (void)uln;

    printf("Progress: %s (%g/%g)\n", conn->url, dlnow, dltotal);
    return 0;
}

/* Create a new easy handle, and add it to the global curl_multi */
static void new_conn(char *url, struct SpiderGlobalInfo *g) {
    struct ConnInfo *conn;
    CURLMcode rc;

    conn = calloc(1, sizeof(struct ConnInfo));
    memset(conn, 0, sizeof(struct ConnInfo));
    conn->error[0] = '\0';

    conn->easy = curl_easy_init();
    if (!conn->easy) {
        printf("curl_easy_init() failed, exiting!\n");
        exit(EXIT_FAILURE);
    }

    conn->global = g;
    conn->url = strdup(url);
    curl_easy_setopt(conn->easy, CURLOPT_URL, conn->url);
    curl_easy_setopt(conn->easy, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(conn->easy, CURLOPT_WRITEDATA, &conn);
    curl_easy_setopt(conn->easy, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(conn->easy, CURLOPT_ERRORBUFFER, conn->error);
    curl_easy_setopt(conn->easy, CURLOPT_PRIVATE, conn);
    curl_easy_setopt(conn->easy, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(conn->easy, CURLOPT_PROGRESSFUNCTION, prog_cb);
    curl_easy_setopt(conn->easy, CURLOPT_PROGRESSDATA, conn);
    curl_easy_setopt(conn->easy, CURLOPT_LOW_SPEED_TIME, 3L);
    curl_easy_setopt(conn->easy, CURLOPT_LOW_SPEED_LIMIT, 10L);

    printf("Adding easy %p to multi %p (%s)\n", conn->easy, g->multi, url);
    rc = curl_multi_add_handle(g->multi, conn->easy);
    mcode_or_die("new_conn: curl_multi_add_handle", rc);

    /* note that the add_handle() will set a time-out to trigger very soon so
       that the necessary socket_action() call will be called by this app */
}

void crawl(struct arguments *arg) {
    struct SpiderGlobalInfo global_info;

    /* Init before looping starts */
    global_info.loop = ev_default_loop(0);
    global_info.multi = curl_multi_init();
    ev_timer_init(&(global_info.timer_event), socket_action_timer_cb, 0., 0.);
    global_info.timer_event.data = &global_info;
    curl_multi_setopt(global_info.multi, CURLMOPT_TIMERFUNCTION, multi_timer_cb);
    curl_multi_setopt(global_info.multi, CURLMOPT_TIMERDATA, &global_info);
    curl_multi_setopt(global_info.multi, CURLMOPT_SOCKETFUNCTION, sock_cb);
    curl_multi_setopt(global_info.multi, CURLMOPT_SOCKETDATA, &global_info);

    new_conn(arg->url, &global_info);

    /* Lets loop */
    ev_loop(global_info.loop, 0);

    /* Cleanups after looping */
    curl_multi_cleanup(global_info.multi);
}

