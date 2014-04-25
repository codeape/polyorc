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

#include "generator.h"
#include "polyorcout.h"
#include "polyorcdefs.h"
#include "polyorctypes.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <ev.h>
#include <curl/curl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

// No lock needed. We only read this.
static unsigned int ring_size;
static char **ring;

// No lock needed. We only indicate if run or not.
static int done;

/* Global information, common to all connections */
typedef struct _global_info {
    int id;
    struct ev_loop *loop;
    struct ev_timer timer_event;
    int still_running;
    CURLM *multi;
    int job_max;
    int job_count;
    int stat_need_sync;
    struct timeval read_time;
    int read_byte_memory;
    int hits_sec;
    orcstatistics *stat;
    int current;
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

/* Used for per thread contex data*/
typedef struct _thread_context {
    int id;
    pthread_t pthread;
    polyarguments *arg;
} thread_context;

/* If we use mmaped memory, sync it */
void sync_mmap_ifused(global_info *global) {
    if (1 == global->stat_need_sync) {
        int res = msync(global->stat, sizeof(orcstatistics), MS_ASYNC);
        if (-1 == res) {
            orcerror("Sync of maped memory failed\n");
            orcerrno(errno);
            exit(EXIT_FAILURE);
        }
    }
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

static void new_conn(global_info *global);

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
            /*if (200 == response_code || 200 == connect_code) {
                //remove this block?
            }*/
            /* Create new readers here */
            if (0 == done) {
                orcout(orcm_debug, "T%d --> %s\n", global->id, conn->url);
                new_conn(global);
            }
            global->stat->hits++;
            global->hits_sec++;
            /* Cleanups after download */
            free(conn->memory);
            free(conn);
        }
    }
}

/* Called by libevent when our "wait for socket actions" timeout expires */
static void socket_action_timer_cb(struct ev_loop *loop, struct ev_timer *timer, int revents) {
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
static void event_cb(struct ev_loop *loop, struct ev_io *event_io, int revents) {
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

    /* lets update the */
    conn->global->stat->total_bytes += realsize;
    conn->global->read_byte_memory += realsize;

    struct timeval readt;
    readt.tv_sec = conn->global->read_time.tv_sec;
    readt.tv_usec = conn->global->read_time.tv_usec;
    struct timeval now;
    gettimeofday(&now, 0);
    if ((now.tv_sec - readt.tv_sec) >= 2) {
        long sec = now.tv_sec - readt.tv_sec;
        long usec = now.tv_usec - readt.tv_usec;
        if (usec < 0) {
            sec--;
            usec = 1000000 - (readt.tv_usec - now.tv_usec);
        }
        double timediff = (double)sec + ((double)usec / 1000000.0);

        conn->global->stat->bytes_sec =
            conn->global->read_byte_memory / timediff;
        conn->global->read_byte_memory = 0;
        conn->global->stat->hits_sec = conn->global->hits_sec;
        conn->global->hits_sec = 0;
        printf("%f %d\n", timediff, conn->global->stat->bytes_sec);
        gettimeofday(&(conn->global->read_time), 0);
        //sync_mmap_ifused(conn->global);
    }

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
    return done;
}

/* Create a new easy handle, and add it to the global curl_multi */
static void new_conn(global_info *global) {
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
    conn->url = ring[global->current];
    global->current++;
    if (ring_size == global->current) {
        global->current = 0;
    }
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

    orcout(orcm_debug, "Adding easy %p to multi %p (%s)\n", conn->easy,
           global->multi, conn->url);
    rc = curl_multi_add_handle(global->multi, conn->easy);
    mcode_or_die("new_conn: curl_multi_add_handle", rc);

    global->job_count++;
    /* note that the add_handle() will set a time-out to trigger very soon so
       that the necessary socket_action() call will be called by this app */
}

void * event_loop(void *ptr) {
    thread_context *context = (thread_context *)ptr;

    global_info global;
    orcstatistics altstat;

    memset(&global, 0, sizeof(global_info));

    global.stat = &altstat;
    char path[PATH_MAX - 1];
    if (0 != context->arg->stat_dir) {
        int len = snprintf(path, PATH_MAX - 2, "%s/%d.threadmem",
                           context->arg->stat_dir, context->id);
        if (-1 == len) {
            orcerror("Name error for path %s", path);
            orcerrno(errno);
            exit(EXIT_FAILURE);
        }
        int fd = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR);
        if (-1 == fd) {
            orcerror("File %s", path);
            orcerrno(errno);
            exit(EXIT_FAILURE);
        }

        /* make the file big enough for the mmaped memory */
        ftruncate(fd, sizeof(orcstatistics));

        global.stat = mmap(NULL, sizeof(orcstatistics), PROT_READ | PROT_WRITE,
                           MAP_SHARED, fd, 0);
        if (MAP_FAILED == global.stat) {
            orcerror("Maping memory failed \n");
            orcerrno(errno);
            exit(EXIT_FAILURE);
        }
        fd = close(fd);
        if (-1 == fd) {
            orcerror("Close file %s", path);
            orcerrno(errno);
            exit(EXIT_FAILURE);
        }
        global.stat_need_sync = 1;
    }
    memset(global.stat, 0, sizeof(orcstatistics));
    global.stat->thread_no = context->id;

    // Let us start at a random place in the ring
    global.current = random()%ring_size;

    global.id = context->id;
    global.job_max = context->arg->max_events;
    global.loop = ev_loop_new(0);
    global.multi = curl_multi_init();
    ev_timer_init(&(global.timer_event), socket_action_timer_cb, 0., 0.);
    global.timer_event.data = &global;
    curl_multi_setopt(global.multi, CURLMOPT_TIMERFUNCTION, multi_timer_cb);
    curl_multi_setopt(global.multi, CURLMOPT_TIMERDATA, &global);
    curl_multi_setopt(global.multi, CURLMOPT_SOCKETFUNCTION, sock_cb);
    curl_multi_setopt(global.multi, CURLMOPT_SOCKETDATA, &global);

    gettimeofday(&(global.read_time), 0);
    new_conn(&global);
    ev_loop(global.loop, 0);

    /* Cleanups after looping */
    curl_multi_cleanup(global.multi);
    return 0;
}

static void finish(int sig)
{
    done = 1;
    orcoutc(orc_reset, orc_red, "\nCtrl+c detected!\n");
}

void generator_loop(polyarguments *arg) {
    // Add Ctrl+c handling
    done = 0;
    signal(SIGINT, finish);

    thread_context event_threads[arg->max_threads];
    int i;
    for (i = 0; i < arg->max_threads; i++) {
        event_threads[i].id = i + 1;
        event_threads[i].arg = arg;
        int status = pthread_create(&(event_threads[i].pthread),
                                    0,
                                    event_loop,
                                    (void *)&event_threads[i]);
        if (0 == status) {
            orcstatus(orcm_normal, orc_green, "STARTED", "Thread %d\n",
                      event_threads[i].id);
        } else {
            orcerror("Thread %d %s (%d)\n", event_threads[i].id,
                     strerror(status), status);
            exit(status);
        }
    }

    for (i = 0; i < arg->max_threads; i++) {
        pthread_join(event_threads[i].pthread, 0);
        orcstatus(orcm_normal, orc_green, "HALTED", "Thread %d\n",
                  event_threads[i].id);
    }
}

void create_url_ring(const char* file_name) {
    ring_size = 0;
    ring = 0;
    char *buff = 0;
    size_t buff_len = 0;
    int status = 0;

    FILE *url_file = fopen(file_name, "r");
    if (0 == url_file) {
        orcerror("%s (%d)\n", strerror(errno), errno);
    }

    int i = 0;
    while (0 < (status = getline(&buff, &buff_len, url_file))) {
        i++;
    }
    if (-1 == status && 0 < errno) {
        orcerror("%s (%d)\n", strerror(errno), errno);
    }
    if (0 == i) {
        orcout(orcm_quiet, "No urls to process!\n");
        exit(0);
    }

    const int lines = i;
    ring_size = i;
    ring = calloc(i, sizeof(char*));
    if (0 == ring) {
        orcerror("%s (%d)\n", strerror(errno), errno);
        exit(EXIT_FAILURE);
    }

    rewind(url_file);
    i = 0;
    while (i < lines && 0 < (status = getline(&buff, &buff_len, url_file))) {
        int index = buff_len - 1;
        while (index >= 0) {
            if(buff[index] == '\n') {
                buff[index] = '\0';
                break;
            }
            index--;
        }
        ring[i] = buff;
        buff = 0;
        i++;
    }
    if (-1 == status && 0 < errno) {
        orcerror("%s (%d)\n", strerror(errno), errno);
    }
}

void generator_init(polyarguments *arg) {
    create_url_ring(arg->in_file);
}

void generator_destroy(){
    if (0 != ring) {
        free(ring);
    }
}

