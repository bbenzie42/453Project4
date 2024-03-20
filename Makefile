CC = gcc
CFLAGS = -Wall -g -std=c99 -Wl,--allow-multiple-definition
PROG = tinyFSDemo
OBJS = tinyFSDemo.o libTinyFS.o libDisk.o

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS)

tinyFSDemo.o: libDisk.c TinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<

libTinyFS.o: libTinyFS.c libDisk.h libDisk.o TinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<

libDisk.o: libDisk.c libDisk.h  TinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<