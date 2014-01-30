Polyorc
============

This will become a http load generator tool suite. It will contain the following
tools

+ polyorc - Http load generator
+ polyorcboss - A tool for displaying polyorc stats
+ polyorcspider - A spider that uses parallel downloads

Dependencies
============

+ uriparser (http://uriparser.sourceforge.net/)
+ libcurl (http://curl.haxx.se/libcurl/)
+ libev (http://software.schmorp.de/pkg/libev.html)
+ argp (http://www.gnu.org/software/libc/manual/html_node/Argp.html)

Some goals
============

+ A load generator that uses multi threading and event loops
+ Linux, Mac OS X and FreeBSD

Build
============

Polyorc uses Waf as build system. Waf depends on python 2.4 or later. To build
the project:

         ./waf distclean configure build

Usage
============

To get help you can always use the --help option on polyorc, polyorcboss or
polyorcspider.

Right now there is no install target in the build. After build just run the
commands from this directory.

To run the spider in color with verbose output excluding all anchor url on
www.example.com

        ./build/polyorcspider/polyorcspider -c -v --exclude="#" http://www.example.com/

Polyorcspider will put all urls it finds in a file called spider.out.

To generate traffic with the urls in spider.out run polyorc like this:

        ./build/polyorc/polyorc -s /tmp/spdr -f spider.out

The -s flag will mmap a file per thread in the directory path given as argument
and write statistics to it. Now we have traffic, lets open another terminal
and watch some statistics. In the new terminal run the following command.

        ./build/polyorcboss/polyorcboss -s /tmp/spdr/

Polyorcboss will mmap all the files in the directory of the path given to the -s
flag. Polyorcboss will read the statistics in the mmaped files and show them in
a ncurses gui.

