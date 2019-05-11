CC = gcc
CFLAGS = -I. -Wall
LIBS =  -lncursesw
SRCS = cf.c
OBJS = $(SRCS: .c = .o)
PROG = cfiles

prefix = /usr/local
bindir = $(prefix)/bin
mandir = $(prefix)/man

BINDIR = $(DESTDIR)/$(bindir)
MANDIR = $(DESTDIR)/$(mandir)

all: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(PROG) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm *.o
	rm *~

install:
	install -Dm 755 $(PROG) $(BINDIR)/$(PROG)
	install -Dm 755 scripts/clearimg $(BINDIR)/clearimg
	install -Dm 755 scripts/displayimg $(BINDIR)/displayimg
	install -Dm 755 scripts/displayimg_uberzug $(BINDIR)/displayimg_uberzug
	install -Dm 755 scripts/clearimg_uberzug $(BINDIR)/clearimg_uberzug
	install -Dm 644 cfiles.1 $(MANDIR)/man1/cfiles.1

uninstall:
	rm -v $(BINDIR)/$(PROG)
	rm -v $(BINDIR)/clearimg
	rm -v $(BINDIR)/clearimg_uberzug
	rm -v $(BINDIR)/displayimg_uberzug
	rm -v $(BINDIR)/displayimg
	rm -v /usr/share/licenses/$(PROG)/LICENSE
	rm -v $(MANDIR)/man1/cfiles.1