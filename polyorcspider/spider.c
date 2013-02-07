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

/*
 This part of the code is very inspired by the libcurl examples evhiperfifo.c
 and getinmemory.c
*/

#include "spider.h"
#include "polyorcbintree.h"
#include "polyorcutils.h"
#include "polyorcmatcher.h"
#include "polyorcout.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <ev.h>
#include <sys/time.h>

/* A url fifo node*/
typedef struct _url_node {
   char *url;
   struct _url_node *next;
} url_node;

typedef struct _url_info {
    int dead;
    int found_count;
} url_info;

/* Global information, common to all connections */
typedef struct _global_info {
    struct ev_loop *loop;
    struct ev_timer timer_event;
    int still_running;
    CURLM *multi;
    find_urls_input input;
    int job_max;
    int job_count;
    url_node *url_out;
    url_node *url_in;
    bintree_root url_tree;
    const char *out_name;
    FILE *out;
    long total_bytes;
} global_info;

/* Information associated with a specific easy handle */
typedef struct _conn_info {
    CURL *easy;
    char *url;
    global_info *global;
    char *memory;
    size_t memory_size;
    char error[CURL_ERROR_SIZE];
} conn_info;

static void new_conn(char *, global_info *);

/* Information associated with a specific socket */
typedef struct _sock_info {
    curl_socket_t sockfd;
    CURL *easy;
    int action;
    long timeout;
    struct ev_io ev;
    int evset;
    global_info *global;
} sock_info;

/* Adds a url to the head of the fifo */
static void url_add(global_info *global, char *url) {
   url_node *newnode = calloc(1, sizeof(url_node));
   if (0 == newnode) {
      orcerror("%s (%d)\n", strerror(errno), errno);
      exit(EXIT_FAILURE);
   }
   newnode->url = url;
   if (0 == global->url_out) {
      // The empty list
      global->url_out = newnode;
      global->url_in = newnode;
   } else {
      global->url_in->next = newnode;
      global->url_in = newnode;
   }
}

/* Fetches a url from the tail of the fifo */
static char * url_get(global_info *global) {
   if (0 == global->url_out) {
      return 0;
   }
   url_node *old = global->url_out;
   global->url_out = global->url_out->next;
   char *ret = old->url;
   free(old);
   return ret;
}

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
            orcerror("%s returns %s\n", where, s);
            /* ignore this error */
            return;
        }
        orcerror("%s returns %s\n", where, s);
        exit(code);
    }
}

/* Find urls in a page and keep them */
static void analyze_page(global_info *global, conn_info *conn) {
    /* Analyze */
    int matches = 0;
    global->input.url = conn->url;
    if(-1 == (matches = find_urls(conn->memory, &(global->input))))
    {
        if (0 != global->input.ret_len) {
            free_array_of_charptr_incl(&(global->input.ret),
                                       global->input.ret_len);
        }
        exit(EXIT_FAILURE);
    }
    global->input.url = 0;

    /* Save */
    int i;
    for (i = 0; i < matches; i++) {
        url_add(global, global->input.ret[i]);
        global->input.ret[i] = 0;
    }
}

static void read_new_pages(global_info *global) {
    int max_count = global->job_max;
    char* url = 0;
    while (global->job_count <= max_count && 0 != (url = url_get(global))) {
        url_info *info = 0;
        if (0 != (info = (url_info *)bintree_find(&(global->url_tree), url))) {
            info->found_count++;
            orcstatus(orcm_debug, orc_cyan, "counted", "%s\n", url);
            free(url);
        } else {
            info = calloc(1, sizeof(*info));
            info->found_count++;
            bintree_add(&(global->url_tree), url, info);
            new_conn(url, global);
        }
    }
}

/* Check for completed transfers, and remove their easy handles */
static void check_multi_info(global_info *global) {
    conn_info *conn;
    char *effective_url;
    CURL *easy;
    CURLcode result;
    CURLMsg *msg;
    int msgs_left;
    long response_code;
    long connect_code;

    orcout(orcm_debug, "REMAINING: %d\n", global->still_running);
    while ((msg = curl_multi_info_read(global->multi, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            easy = msg->easy_handle;
            result = msg->data.result;
            /* Retrive the connection info for a handle */
            curl_easy_getinfo(easy, CURLINFO_PRIVATE, &conn);
            curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &effective_url);
            curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &response_code);
            curl_easy_getinfo(easy, CURLINFO_HTTP_CONNECTCODE, &connect_code);
            orcout(orcm_debug, "response code:%ld connect_code:%ld\n", response_code, connect_code);
            orcout(orcm_debug, "DONE: %s => (%d) %s\n", effective_url,
                   result, conn->error);
            orcout(orcm_debug, "%s", conn->memory);
            /* Cleanup the finished easy handle */
            curl_multi_remove_handle(global->multi, easy);
            curl_easy_cleanup(easy);
            /* Write visited url to file */
            global->job_count--;
            if (200 == response_code || 200 == connect_code) {
                orcstatus(orcm_verbose, orc_green, "added", "%s\n", conn->url);
                if(0 > fprintf(global->out, "%s\n", conn->url)) {
                    orcerror("%s (%d) %s\n", strerror(errno), errno,
                             global->out_name);
                    exit(EXIT_FAILURE);
                }
                /* Analyze here */
                analyze_page(global, conn);
                /* Collect stats */
                global->total_bytes += conn->memory_size;
            } else {
                /* Mark as dead */
                url_info *info = 0;
                if (0 != (info = (url_info *)
                          bintree_find(&(global->url_tree), conn->url)))
                {
                    info->dead = 1;
                }
                orcstatus(orcm_verbose, orc_red, "dead", "%s\n", conn->url);
            }
            /* Create new readers here */
            read_new_pages(global);
            /* Cleanups after download */
            free(conn->url);
            free(conn->memory);
            free(conn);
        }
    }
}

/* Called by libevent when our "wait for socket actions" timeout expires */
static void socket_action_timer_cb(EV_P_ struct ev_timer *timer, int revents) {
    orcout(orcm_debug, "%s  timer %p revents %i\n", __PRETTY_FUNCTION__,
           timer, revents);
    global_info *global = (global_info *)timer->data;
    CURLMcode rc;

    /* Do the timeout action */
    rc = curl_multi_socket_action(global->multi, CURL_SOCKET_TIMEOUT, 0,
                                  &(global->still_running));
    mcode_or_die("socket_action_timer_cb: curl_multi_socket_action", rc);
    check_multi_info(global);
}

/* Update the event timer ("wait for socket actions") after curl_multi library
   calls */
static int multi_timer_cb(CURLM *multi, long timeout_ms, global_info *global) {
    orcout(orcm_debug, "%s timeout %li\n", __PRETTY_FUNCTION__,  timeout_ms);
    ev_timer_stop(global->loop, &(global->timer_event));
    if (timeout_ms > 0) {
        double  t = timeout_ms / 1000;
        ev_timer_init(&(global->timer_event), socket_action_timer_cb, t, 0.);
        ev_timer_start(global->loop, &(global->timer_event));
    } else {
        socket_action_timer_cb(global->loop, &(global->timer_event), 0);
    }
    return 0;
}

/* Called by libevent when we get action on a multi socket */
static void event_cb(EV_P_ struct ev_io *event_io, int revents) {
    orcout(orcm_debug, "%s  event_io %p revents %i\n", __PRETTY_FUNCTION__,
           event_io, revents);
    global_info *global = (global_info *)event_io->data;
    CURLMcode rc;

    int action = (revents & EV_READ ? CURL_POLL_IN : 0) |
          (revents & EV_WRITE ? CURL_POLL_OUT : 0);
    rc = curl_multi_socket_action(global->multi, event_io->fd, action,
                                  &(global->still_running));
    mcode_or_die("event_cb: curl_multi_socket_action", rc);
    check_multi_info(global);
    if (global->still_running <= 0 && global->job_count  <= 0) {
        orcout(orcm_debug, "last transfer done, kill timeout\n");
        ev_timer_stop(global->loop, &(global->timer_event));
    }
}

/* Clean up the SockInfo structure */
static void remsock(sock_info *soc, global_info *global) {
    if (soc) {
        if (soc->evset) {
            ev_io_stop(global->loop, &(soc->ev));
        }
        free(soc);
    }
}

/* Assign information to a SockInfo structure */
static void setsock(sock_info *soc, curl_socket_t curl_soc, CURL *handle,
                    int action, global_info *global)
{
    int kind = (action & CURL_POLL_IN ? EV_READ : 0) |
          (action & CURL_POLL_OUT ? EV_WRITE : 0);

    soc->sockfd = curl_soc;
    soc->action = action;
    soc->easy = handle;
    if (soc->evset) {
        ev_io_stop(global->loop, &(soc->ev));
    }
    ev_io_init(&(soc->ev), event_cb, soc->sockfd, kind);
    soc->ev.data = global;
    soc->evset = 1;
    ev_io_start(global->loop, &(soc->ev));
}

/* Initialize a new SockInfo structure */
static void addsock(curl_socket_t curl_soc, CURL *handle, int action,
                    global_info *global)
{
    sock_info *soc = calloc(sizeof(sock_info), 1);

    soc->global = global;
    setsock(soc, curl_soc, handle, action, global);
    curl_multi_assign(global->multi, curl_soc, soc);
}

/* Notifies about updates on a socket file descriptor */
static int sock_cb(CURL *handle, curl_socket_t curl_soc, int what, void *cbp,
                   void *sockp)
{
    orcout(orcm_debug, "%s handle %p curl_soc %i what %i cbp %p sockp %p\n",
         __PRETTY_FUNCTION__, handle, curl_soc, what, cbp, sockp);
    global_info *global = (global_info *)cbp;
    sock_info *soc = (sock_info *)sockp;
    const char *whatstr[] = { "none", "IN", "OUT", "INOUT", "REMOVE" };

    orcout(orcm_debug, "socket callback: s=%d e=%p what=%s ",
           curl_soc, handle, whatstr[what]);
    if (what == CURL_POLL_REMOVE) {
        orcout(orcm_debug, "\n");
        remsock(soc, global);
    } else {
        if (!soc) {
            orcout(orcm_debug, "Adding data: %s\n", whatstr[what]);
            addsock(curl_soc, handle, what, global);
        } else {
            orcout(orcm_debug, "Changing action from %s to %s\n",
                   whatstr[soc->action], whatstr[what]);
            setsock(soc, curl_soc, handle, what, global);
        }
    }
    return 0;
}

/* CURLOPT_WRITEFUNCTION - reads a page and write to memmory */
static size_t write_cb(void *contents, size_t size, size_t nmemb, void *data) {
    size_t realsize = size * nmemb;
    conn_info *conn = (conn_info *)data;

    conn->memory = realloc(conn->memory, conn->memory_size + realsize + 1);
    if (0 == conn->memory) {
        /* out of memory! */
        orcerror("%s (%d)\n", strerror(errno), errno);
        exit(EXIT_FAILURE);
    }

    memcpy(&(conn->memory[conn->memory_size]), contents, realsize);
    conn->memory_size += realsize;
    conn->memory[conn->memory_size] = 0;

    return realsize;
}

/* CURLOPT_PROGRESSFUNCTION */
static int prog_cb(void *data, double dltotal, double dlnow, double ult,
                   double uln) {
    conn_info *conn = (conn_info *)data;
    (void)ult;
    (void)uln;

    if (dlnow > 0 && dltotal > 0) {
        orcout(orcm_debug, "Progress: %s (%g/%g)\n", conn->url, dlnow, dltotal);
    }
    return 0;
}

/* Create a new easy handle, and add it to the global curl_multi */
static void new_conn(char *url, global_info *global) {
    CURLMcode rc;
    conn_info *conn;
    long int debug = 1L;
    if (orcm_debug != debug) {
        debug = 0;
    }

    conn = calloc(1, sizeof(conn_info));
    memset(conn, 0, sizeof(conn_info));

    conn->error[0] = '\0';
    conn->easy = curl_easy_init();
    if (!conn->easy) {
        orcerror("curl_easy_init() failed, exiting!\n");
        exit(EXIT_FAILURE);
    }

    conn->memory = calloc(1, sizeof(char));
    conn->memory_size = 0;

    conn->global = global;
    conn->url = strdup(url);
    curl_easy_setopt(conn->easy, CURLOPT_URL, conn->url);
    curl_easy_setopt(conn->easy, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(conn->easy, CURLOPT_WRITEDATA, conn);
    curl_easy_setopt(conn->easy, CURLOPT_VERBOSE, debug);
    curl_easy_setopt(conn->easy, CURLOPT_ERRORBUFFER, conn->error);
    curl_easy_setopt(conn->easy, CURLOPT_PRIVATE, conn);
    curl_easy_setopt(conn->easy, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(conn->easy, CURLOPT_PROGRESSFUNCTION, prog_cb);
    curl_easy_setopt(conn->easy, CURLOPT_PROGRESSDATA, conn);
    curl_easy_setopt(conn->easy, CURLOPT_LOW_SPEED_TIME, 3L);
    curl_easy_setopt(conn->easy, CURLOPT_LOW_SPEED_LIMIT, 10L);
    curl_easy_setopt(conn->easy, CURLOPT_USERAGENT, ORC_USERAGENT);
    curl_easy_setopt(conn->easy, CURLOPT_FOLLOWLOCATION, 1);

    orcout(orcm_debug, "Adding easy %p to multi %p (%s)\n", conn->easy, global->multi, url);
    rc = curl_multi_add_handle(global->multi, conn->easy);
    mcode_or_die("new_conn: curl_multi_add_handle", rc);

    global->job_count++;
    /* note that the add_handle() will set a time-out to trigger very soon so
       that the necessary socket_action() call will be called by this app */
}

static void free_tree(void **key, void **value, const enum free_cmd cmd) {
    free(*key);
    (*key) = 0;
    if (POLY_FREE_ALL == cmd) {
        free(*value);
        (*value) = 0;
    }
}

void print_stats(global_info *global, struct timeval *start,
                 struct timeval *stop) {
    long sec = stop->tv_sec - start->tv_sec;
    long usec = stop->tv_usec - start->tv_usec;
    if (usec < 0) {
        sec--;
        usec = 1000000 - (start->tv_usec - stop->tv_usec);
    }

    orcoutc(orc_reset, orc_red, "Downloaded:     ");
    orcout(orcm_quiet, "%d bytes\n", global->total_bytes);


    orcoutc(orc_reset, orc_red, "Time:           ");
    orcout(orcm_quiet, "%d.%d sec\n", sec, usec);

    orcoutc(orc_reset, orc_red, "Collected urls: ");
    orcout(orcm_quiet, "%d\n", global->url_tree.node_count);
}

void add_first_call(arguments *arg, global_info *global) {
    /* Copy the main url */
    size_t root_url_len = strnlen(arg->url, MAX_URL_LEN + 10);
    if (root_url_len > MAX_URL_LEN) {
        orcerror("%s : url is larger than defacto limit %d\n", arg->url);
        exit(EXIT_FAILURE);
    }
    char *root_url = calloc(root_url_len + 1, sizeof(char));
    if (0 == root_url) {
        orcerror("%s (%d)\n", strerror(errno), errno);
        exit(EXIT_FAILURE);
    }
    /* Create a info value for the main url */
    url_info *info = calloc(1, sizeof(*info));
    if (0 == info) {
        orcerror("%s (%d)\n", strerror(errno), errno);
        exit(EXIT_FAILURE);
    }
    strncpy(root_url, arg->url, root_url_len);
    root_url[root_url_len] = '\0';
    /* Add a connection to the url where we will start the spider */
    new_conn(root_url, global);
    /* root_url and info are freed in bintree_free */
    bintree_add(&(global->url_tree), root_url, info);
}

void crawl(arguments *arg) {
    global_info global;
    memset(&global, 0, sizeof(global_info));
    char search_name[SEARCH_NAME_LEN];
    memset(search_name, '\0', SEARCH_NAME_LEN * sizeof(char));

    /* Init before looping starts */
    global.out_name = arg->out_file;
    if (0 == (global.out = fopen(global.out_name, "w+"))) {
        orcerror("%s (%d) %s\n", strerror(errno), errno, global.out_name);
        exit(EXIT_FAILURE);
    }

    global.job_max = arg->max_jobs;
    global.loop = ev_default_loop(0);
    global.multi = curl_multi_init();
    ev_timer_init(&(global.timer_event), socket_action_timer_cb, 0., 0.);
    global.timer_event.data = &global;
    curl_multi_setopt(global.multi, CURLMOPT_TIMERFUNCTION, multi_timer_cb);
    curl_multi_setopt(global.multi, CURLMOPT_TIMERDATA, &global);
    curl_multi_setopt(global.multi, CURLMOPT_SOCKETFUNCTION, sock_cb);
    curl_multi_setopt(global.multi, CURLMOPT_SOCKETDATA, &global);
    bintree_init(&(global.url_tree), bintree_streq, free_tree);

    global.input.search_name = search_name;
    global.input.search_name_len = SEARCH_NAME_LEN;
    global.input.excludes = arg->excludes;
    global.input.excludes_len = arg->excludes_len;

    if (!find_search_name(arg->url, search_name , SEARCH_NAME_LEN))
    {
        orcerror("not a valid domain name or ip in: %s\n", arg->url);
        exit(EXIT_FAILURE);
    }
    orcoutc(orc_reset, orc_blue, "Target %s\n", global.input.search_name);

    add_first_call(arg, &global);

    struct timeval start;
    struct timeval stop;

    /* Lets find some urls */
    gettimeofday(&start, 0);
    ev_loop(global.loop, 0);
    gettimeofday(&stop, 0);

    print_stats(&global, &start, &stop);

    /* Cleanups after looping */
    fclose(global.out);
    free_array_of_charptr_incl(&(global.input.ret), global.input.ret_len);
    bintree_free(&(global.url_tree));
    curl_multi_cleanup(global.multi);

    char *url_item = 0;
    int i = 0;
    while (0 != (url_item = url_get(&global))) {
        free(url_item);
        i++;
    }
    orcout(orcm_debug, "Freed %d url items.\n", i);
}

