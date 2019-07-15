CC = gcc

NCURSES_CFLAGS = `pkg-config --cflags ncursesw`
NCURSES_LIBS =  `pkg-config --libs ncursesw`

LIBS += $(NCURSES_LIBS)
CFLAGS += $(NCURSES_CFLAGS)

SRCS = cf.c
OBJS = $(SRCS: .c = .o)
PROG = cfiles

prefix = usr
bindir = $(prefix)/bin
scriptdir = $(prefix)/share/cfiles/scripts
mandir = $(prefix)/share/man

BINDIR = $(DESTDIR)/$(bindir)
MANDIR = $(DESTDIR)/$(mandir)
SCRIPTDIR = $(DESTDIR)/$(scriptdir)

all: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(PROG) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm *.o
	rm *~

install:
	install -Dm 755 $(PROG) $(BINDIR)/$(PROG)
	install -Dm 755 scripts/clearimg $(SCRIPTDIR)/clearimg
	install -Dm 755 scripts/displayimg $(SCRIPTDIR)/displayimg
	install -Dm 755 scripts/displayimg_uberzug $(SCRIPTDIR)/displayimg_uberzug
	install -Dm 755 scripts/clearimg_uberzug $(SCRIPTDIR)/clearimg_uberzug
	install -Dm 644 cfiles.1 $(MANDIR)/man1/cfiles.1

uninstall:
	rm -v $(BINDIR)/$(PROG)
	rm -v $(SCRIPTDIR)/clearimg
	rm -v $(SCRIPTDIR)/clearimg_uberzug
	rm -v $(SCRIPTDIR)/displayimg_uberzug
	rm -v $(SCRIPTDIR)/displayimg
	rm -v $(MANDIR)/man1/cfiles.1

