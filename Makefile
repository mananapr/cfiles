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
	cp -v $(PROG) $(DEST)

uninstall:
	rm -v "$(DEST)/$(PROG)"
