/*
 *  Copyright (c) 2010-2011 Csaba Kiraly
 *
 *  This is free software; see COPYING.GPLv3.txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX(a,b) ((a>b) ? a : b)
#define MIN(a,b) ((a>b) ? b : a)

size_t bufsize = 1024*1024;
size_t readsize = 1024 * 32;

void *buf, *rpos, *wpos;

static void usage(int argc, char *argv[])
{
  fprintf(stderr, "This is fifo: a simple replacement for the shell pipe (|)\n"\
                  "Usage:\n"\
                  "%s [-b bufsize] [-r readsize]\n"\
                  "\t-b bufsize: set FIFO buffer size to bufsize bytes\n"\
                  "\t-r readsize: read in blocks of readsize bytes\n",
                  argv[0]
         );
}

static void cmdline_parse(int argc, char *argv[])
{
  int o;

  while ((o = getopt(argc, argv, "b:r:h")) != -1) {
    switch(o) {
      case 'b':
        bufsize = atol(optarg);
      case 'r':
        readsize = atol(optarg);
        break;
      case 'h':
        usage(argc,argv);
        exit(EXIT_SUCCESS);
      default:
        fprintf(stderr, "Error: unknown option %c\n", o);
        usage(argc,argv);
        exit(EXIT_FAILURE);
    }
  }
}

int main(int argc, char *argv[])
{
  fd_set rfds, wfds;
  int reof = 0;
  int retval;
  int flags;

  cmdline_parse(argc, argv);

  flags = fcntl(0, F_GETFL, 0);
  fcntl(0, F_SETFL, flags | O_NONBLOCK);

  flags = fcntl(1, F_GETFL, 0);
  fcntl(1, F_SETFL, flags | O_NONBLOCK);

  buf = rpos = wpos = malloc(bufsize);
  if (!buf) {
    fprintf(stderr, "Error: can't allocate buffer of %lu bytes\n", bufsize);
    exit(EXIT_SUCCESS);
  }

  while(1) {

    /* Watch stdin (fd 0) to see when it has input. */
    FD_ZERO(&rfds);
    if (!reof && rpos != wpos-1 && rpos-bufsize != wpos-1 ) FD_SET(0, &rfds);

    /* Watch stdout (fd 1) to see when it can be written. */
    FD_ZERO(&wfds);
    if (rpos != wpos) FD_SET(1, &wfds);

    retval = select(2, &rfds, &wfds, NULL, NULL);

    if (retval == -1) {
      break;
    } else {
      if (FD_ISSET(0, &rfds)) {
        ssize_t size;
        size_t rmax;
        rmax = MIN(MIN(readsize, buf + bufsize - rpos), (wpos+bufsize-rpos-1) % bufsize);
//fprintf(stderr,"reading %lu bytes, ", rmax);
        size = read(0, rpos, rmax);
//fprintf(stderr,"read %ld bytes\n", size);
        if (size < 0) {
          break;
        } else if (size == 0)  {
          reof = 1;
        } else {
          rpos += size;
          if (rpos >= buf + bufsize) rpos -= bufsize;
        }
      }
      if (FD_ISSET(1, &wfds)) {
        ssize_t size;
        size_t wmax;
        wmax = (wpos > rpos) ? (buf + bufsize) - wpos  : rpos - wpos;
//fprintf(stderr,"writing %lu bytes, ", wmax);
        size = write(1, wpos, wmax);
//fprintf(stderr,"written %ld bytes\n", size);
        if (size < 0) {
          break;
        } else {
          wpos += size;
          if (wpos >= buf + bufsize) wpos -= bufsize;
        }
      }
    }
    if (reof && rpos == wpos) break;
  }
  free(buf);
  exit(EXIT_SUCCESS);
}