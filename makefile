CC=g++
CCFLAGS=-g -std=c++11 -D_BSD_SOURCE -Wall

TARGETS=207httpd

all: $(TARGETS)

207httpd: 207httpd.cpp common.h
	$(CC) $(CCFLAGS) -pthread -o $@ $<

clean:
	rm -f *.o $(TARGETS)

