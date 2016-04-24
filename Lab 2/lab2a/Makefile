# CS 111 Lab1a Makefile
# Project-2A : Spring 2016
# Arjun 504078752

UID = 504078752

CC = gcc
CFLAGS = -g -Wall -O2 -Wextra -Wno-unused -Werror -lrt -lmcrypt
LDLIBS = -lm
LDFLAGS = -pthread
LAB = 2a
DISTDIR = lab$(LAB)-$(UID)

all: lab$(LAB)

SRC = lab$(LAB).c

OBJ = $(subst .c,.o,$(SRC))


DIST_SOURCES = lab$(LAB) \
  $(OBJ) Makefile README lab$(LAB).c

lab$(LAB): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ) $(LDLIBS)

dist: $(DISTDIR).tar.gz

$(DISTDIR).tar.gz: $(DIST_SOURCES)
	@rm -rf $(DISTDIR)
	@export GZIP=-9
	@tar -cvzf $@.tmp --transform='s,^,$(DISTDIR)/,' $(DIST_SOURCES)
	@mv $@.tmp $@
	@echo "tarball created :: $@"

clean:
	rm -rf *.log *.o *~ *.bak *.tar.gz core *.core *.tmp lab$(LAB) $(DISTDIR)

.PHONY: all dist check clean
