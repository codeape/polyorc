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

#include "common.h"
#include "client.h"
#include "polyorcsockutil.h"
#include "polyorctypes.h"

typedef struct _mmapfile {
    int fd;
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

void display_data() {
    int row = 1;
    mmapfile *ptr = files;
    if (ptr == 0) {
        mvprintw(row, 0, "No threads found!\n");
        return;
    }
    while (0 != ptr) {
        mvprintw(row, 0, "Thread %d Bytes: %d\n", ptr->stat->thread_no,
                 ptr->stat->total_bytes);
        ptr = ptr->next;
        row++;
    }
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
}

void client_loop(bossarguments *arg) {
    openfiles(arg->stat_dir);

    init_curses();
    int run = 1;
    struct timeval lasttime;
    struct timeval nowtime;
    gettimeofday(&lasttime, 0);
    while (1 == run) {
        gettimeofday(&nowtime, 0);
        if (( 0 < nowtime.tv_sec - lasttime.tv_sec) ||
            ( 300000 < nowtime.tv_usec - lasttime.tv_usec))
        {
            display_data();
            gettimeofday(&nowtime, 0);
        }

        int ch = getch();
        if ('q' == ch) {
            finish(0);
        }
    }
}
