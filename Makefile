CC = gcc
CFLAGS = -I. -Wall
LIBS =  -lncurses
SRCS = cf.c
OBJS = $(SRCS: .c = .o)
PROG = cfiles
DEST = /usr/local/bin

all: $(OBJS)
	$(CC) $(CFLAGS) $(LIBS) $(OBJS) -o $(PROG)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm *.o
	rm *~

install:
	cp -v $(PROG) $(DEST)

uninstall:
	rm -v "$(DEST)/$(PROG)"
