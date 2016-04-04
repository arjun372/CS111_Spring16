# CS 111 Lab1 Makefile
# Project-0 : Spring 2016
# Arjun 504078752

CC = gcc
CFLAGS = -g -Wall -O2 -Wextra -Wno-unused -Werror
LAB = 0
DISTDIR = lab$(LAB)-arjun

all: lab1

SCREENSHOTS = $(wildcard screenshot*.png)
TESTS = $(wildcard test*.sh)
TEST_BASES = $(subst .sh,,$(TESTS))

SOURCES = \
  alloc.c \
  main.c \
  read-command.c \
  print-command.c

OBJECTS = $(subst .c,.o,$(SOURCES))

DIST_SOURCES = \
  $(OBJECTS) alloc.h command.h command-internals.h Makefile \
  $(TESTS) $(SCREENSHOTS) check-dist README

lab1: $(OBJECTS) $(CC) $(CFLAGS) -o $@ $(OBJECTS)

alloc.o: alloc.h
execute-command.o main.o print-command.o read-command.o: command.h
execute-command.o print-command.o read-command.o: command-internals.h

dist: $(DISTDIR).tar.gz

$(DISTDIR).tar.gz: $(DIST_SOURCES) check-dist
	rm -rf $(DISTDIR)
	tar -czf $@.tmp --transform='s,^,$(DISTDIR)/,' $(DIST_SOURCES)
	./check-dist $(DISTDIR)
	mv $@.tmp $@

check: $(TEST_BASES)

$(TEST_BASES): lab1
	./$@.sh

clean:
	rm -rf *.o *~ *.bak *.tar.gz core *.core *.tmp lab1 $(DISTDIR)

.PHONY: all dist check $(TEST_BASES) clean