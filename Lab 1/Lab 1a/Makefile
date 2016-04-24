# CS 111 Lab1a Makefile
# Project-1a : Spring 2016
# Arjun 504078752

UID = 504078752

CC = gcc
CFLAGS = -g -Wall -O2 -Wextra -Wno-unused -Werror -lrt
LDLIBS = -lm
LDFLAGS = -pthread
LAB = 1a
DISTDIR = lab$(LAB)-$(UID)

all: lab$(LAB)

SOURCES = main.c \

OBJECTS = $(subst .c,.o,$(SOURCES))

DIST_SOURCES = lab$(LAB) \
  $(OBJECTS) Makefile README main.c

lab$(LAB): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

dist: $(DISTDIR).tar.gz

$(DISTDIR).tar.gz: $(DIST_SOURCES)
	@rm -rf $(DISTDIR)
	@tar -cvzf $@.tmp --transform='s,^,$(DISTDIR)/,' $(DIST_SOURCES)
	@mv $@.tmp $@
	@echo "tarball created :: $@"

clean:
	rm -rf *.o *~ *.bak *.tar.gz core *.core *.tmp lab$(LAB) $(DISTDIR)

.PHONY: all dist check clean
