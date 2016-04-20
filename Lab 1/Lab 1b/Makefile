# CS 111 Lab1a Makefile
# Project-1b : Spring 2016
# Arjun 504078752

UID = 504078752

CC = gcc
CFLAGS = -g -Wall -O2 -Wextra -Wno-unused -Werror -lrt -lmcrypt
LDLIBS = -lm
LDFLAGS = -pthread
LAB = 1b
DISTDIR = lab$(LAB)-$(UID)

all: lab$(LAB)

CLI_SRC = client.c
SRV_SRC = server.c

CLI_OBJ = $(subst .c,.o,$(CLI_SRC))

SRV_OBJ = $(subst .c,.o,$(SRV_SRC))

DIST_SOURCES = client server  \
  $(CLI_OBJ) $(SRV_OBJ) Makefile README server.c client.c

lab$(LAB): client server

client: $(CLI_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(CLI_OBJ) $(LDLIBS)

server: $(SRV_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRV_OBJ) $(LDLIBS)

dist: $(DISTDIR).tar.gz

$(DISTDIR).tar.gz: $(DIST_SOURCES)
	@rm -rf $(DISTDIR)
	@export GZIP=-9
	@tar -czf $@.tmp --transform='s,^,$(DISTDIR)/,' $(DIST_SOURCES)
	@mv $@.tmp $@
	@echo "tarball created :: $@"

clean:
	rm -rf *.log *.o *~ *.bak *.tar.gz core *.core *.tmp client server $(DISTDIR)

.PHONY: all dist check clean
