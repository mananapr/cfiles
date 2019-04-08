CC = gcc
CFLAGS = -I. -Wall
LIBS =  -lncursesw
SRCS = cf.c
OBJS = $(SRCS: .c = .o)
PROG = cfiles
DEST = /usr/local/bin

all: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(PROG) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm *.o
	rm *~

install:
	install -Dm 755 $(PROG) $(DEST)/$(PROG)
	install -Dm 755 scripts/clearimg $(DEST)/clearimg
	install -Dm 755 scripts/displayimg $(DEST)/displayimg
	install -Dm 755 scripts/displayimg_uberzug $(DEST)/displayimg_uberzug
	install -Dm 755 scripts/clearimg_uberzug $(DEST)/clearimg_uberzug
	install -Dm 644 LICENSE /usr/share/licenses/$(PROG)/LICENSE
	install -Dm 644 cfiles.1 /usr/local/man/man1/cfiles.1

uninstall:
	rm -v $(DEST)/$(PROG)
	rm -v $(DEST)/clearimg
	rm -v $(DEST)/clearimg_uberzug
	rm -v $(DEST)/displayimg_uberzug
	rm -v $(DEST)/displayimg
	rm -v /usr/share/licenses/$(PROG)/LICENSE
	rm -v /usr/local/man/man1/cfiles.1
