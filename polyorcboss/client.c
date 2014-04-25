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

#include <curses.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <time.h>


#include "common.h"
#include "client.h"
#include "polyorctypes.h"

#define RED_ON_BLACK 1
#define GREEN_ON_BLACK 2
#define YELLOW_ON_BLACK 3
#define BLUE_ON_BLACK 4
#define MAGENTA_ON_BLACK 5
#define CYAN_ON_BLACK 6
#define WHITE_ON_BLACK 7

typedef struct _mmapfile {
    int fd;
    int working;
    int working_check;
    unsigned long long history_total_bytes;
    orcstatistics *stat;
    struct _mmapfile *next;
} mmapfile;

static mmapfile *files = 0;

void openfiles(const char* dir_path) {
    DIR *d;
    struct dirent *dir;
    d = opendir(dir_path);

    if (d)
    {
        const char *suffix = ".threadmem";
        const int suffix_len = strlen(suffix);
        while ((dir = readdir(d)) != NULL)
        {
            const char *file_name = dir->d_name;
            int file_name_len = strlen(file_name);
            if (file_name_len < suffix_len) {
                continue;
            }
            if (0 != strncmp(file_name + (file_name_len - suffix_len),
                             suffix, suffix_len))
            {
                continue;
            }
            mmapfile *file = calloc(1, sizeof(mmapfile));
            char path[PATH_MAX - 1];
            int len = snprintf(path, PATH_MAX - 2, "%s/%s",
                               dir_path, file_name);
            if (-1 == len) {
                orcerror("Name error for path %s", path);
                orcerrno(errno);
                exit(EXIT_FAILURE);
            }
            int fd = open(path, O_RDONLY);
            if (-1 == fd) {
                orcerror("File %s", path);
                orcerrno(errno);
                exit(EXIT_FAILURE);
            }
            file->fd = fd;
            file->fd = 0;
            file->stat = mmap(NULL, sizeof(orcstatistics), PROT_READ,
                          MAP_SHARED, fd, 0);
            if (MAP_FAILED == file->stat) {
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
            if (0 == files) {
                files = file;
            } else {
                mmapfile *ptr = files;
                mmapfile *tail = files;

                while (0 != ptr) {
                    if (ptr->stat->thread_no < file->stat->thread_no) {
                        tail = ptr;
                        ptr = ptr->next;
                    } else {
                        if (tail == ptr) {
                            file->next = ptr;
                            files = file;
                            break;
                        }
                        tail->next = file;
                        file->next = ptr;
                        break;
                    }
                }
                if (0 == ptr) {
                    tail->next = file;
                }
            }
        }
        closedir(d);
    }
}

void display_header() {
    attron(COLOR_PAIR(GREEN_ON_BLACK));
    mvprintw(0, 0, "Polyorc Boss");
    attroff(COLOR_PAIR(GREEN_ON_BLACK));
}

void display_data() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int height = w.ws_row;
    //int width = w.ws_col;

    int row = 2;
    mmapfile *ptr = files;
    clear();
    display_header();
    if (ptr == 0) {
        mvprintw(row, 0, "No threads found!");
        return;
    }
    unsigned long long sum = 0;
    unsigned long long sum_bsec = 0;
    unsigned int sum_hits = 0;
    unsigned int sum_hits_sec = 0;
    while (0 != ptr) {
        mvprintw(row, 0, "Thread %d", ptr->stat->thread_no);

        ptr->working_check++;
        if (ptr->working_check % 10 == 0) {
            if (ptr->stat->total_bytes > ptr->history_total_bytes) {
                ptr->working = 1;
                ptr->history_total_bytes = ptr->stat->total_bytes;
            } else {
                ptr->working = 0;
            }
        }

        if (1 == ptr->working) {
            attron(COLOR_PAIR(GREEN_ON_BLACK));
            mvprintw(row, 10, "work");
            attroff(COLOR_PAIR(GREEN_ON_BLACK));
        } else {
            attron(COLOR_PAIR(RED_ON_BLACK));
            mvprintw(row, 10, "idle");
            attroff(COLOR_PAIR(RED_ON_BLACK));
        }

        mvprintw(row, 16, "%.2Lf %s",
                 byte_to_human_size(ptr->stat->total_bytes),
                 byte_to_human_suffix(ptr->stat->total_bytes));

        mvprintw(row, 36, "%.2Lf %s/s",
                 byte_to_human_size(ptr->stat->bytes_sec),
                 byte_to_human_suffix(ptr->stat->bytes_sec));

        mvprintw(row, 56, "%d d/s", ptr->stat->hits_sec);
        mvprintw(row, 65, "%d d", ptr->stat->hits);

        sum += ptr->stat->total_bytes;
        sum_bsec += ptr->stat->bytes_sec;
        sum_hits += ptr->stat->hits;
        sum_hits_sec += ptr->stat->hits_sec;

        ptr = ptr->next;
        row++;
    }
    mvprintw(height - 1, 0, "Sum");
    mvprintw(height - 1, 16, "%.2Lf %s", byte_to_human_size(sum),
                         byte_to_human_suffix(sum));
    mvprintw(height - 1, 36, "%.2Lf %s/s", byte_to_human_size(sum_bsec),
                         byte_to_human_suffix(sum_bsec));
    mvprintw(height - 1, 56, "%d d/s", sum_hits_sec);
    mvprintw(height - 1, 65, "%d d", sum_hits);
}

static void finish(int sig)
{
    endwin();

    /* do your non-curses wrapup here */

    exit(0);
}

void init_curses() {
    signal(SIGINT, finish);      /* arrange interrupts to terminate */

    initscr();      /* initialize the curses library */
    keypad(stdscr, TRUE);  /* enable keyboard mapping */
    nonl();         /* tell curses not to do NL->CR/NL on output */
    cbreak();       /* take input chars one at a time, no wait for \n */
    notimeout(stdscr, TRUE);
    noecho();
    nodelay(stdscr, TRUE);

    if(has_colors() == TRUE) {
        start_color();
        init_pair(RED_ON_BLACK, COLOR_RED, COLOR_BLACK);
        init_pair(GREEN_ON_BLACK, COLOR_GREEN, COLOR_BLACK);
        init_pair(YELLOW_ON_BLACK, COLOR_YELLOW, COLOR_BLACK);
        init_pair(BLUE_ON_BLACK, COLOR_BLUE, COLOR_BLACK);
        init_pair(MAGENTA_ON_BLACK, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(CYAN_ON_BLACK, COLOR_CYAN, COLOR_BLACK);
        init_pair(WHITE_ON_BLACK, COLOR_WHITE, COLOR_BLACK);
    }
}

void client_loop(bossarguments *arg) {
    openfiles(arg->stat_dir);

    init_curses();
    int run = 1;
    struct timeval lasttime;
    struct timeval nowtime;
    struct timespec sleep_spec;
    sleep_spec.tv_sec = 0;
    sleep_spec.tv_nsec = 300000000L;

    gettimeofday(&lasttime, 0);
    while (1 == run) {
        gettimeofday(&nowtime, 0);
        if (( 0 < nowtime.tv_sec - lasttime.tv_sec) ||
            ( 300000 < nowtime.tv_usec - lasttime.tv_usec))
        {
            display_data();
            gettimeofday(&lasttime, 0);
        }

        int ch = getch();
        if ('q' == ch) {
            finish(0);
        }
        nanosleep(&sleep_spec, 0);
    }
}
