# CS 111 Lab1 Makefile
# Project-0 : Spring 2016
# Arjun 504078752

CC = gcc
CFLAGS = -g -Wall -O2 -Wextra -Wno-unused -Werror
LAB = 0
DISTDIR = lab$(LAB)-arjun

all: lab$(LAB)

SCREENSHOTS = $(wildcard screenshot*.png)

SOURCES = main.c \

OBJECTS = $(subst .c,.o,$(SOURCES))

DIST_SOURCES = \
  $(OBJECTS) Makefile \
  $(SCREENSHOTS) check-dist README

lab$(LAB): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

dist: $(DISTDIR).tar.gz

$(DISTDIR).tar.gz: $(DIST_SOURCES) check-dist
	rm -rf $(DISTDIR)
	tar -czf $@.tmp --transform='s,^,$(DISTDIR)/,' $(DIST_SOURCES)
	./check-dist $(DISTDIR)
	mv $@.tmp $@

check: $(TEST_BASES)

$(TEST_BASES): lab$(LAB)
	./$@.sh

clean:
	rm -rf *.o *~ *.bak *.tar.gz core *.core *.tmp lab$(LAB) $(DISTDIR)

.PHONY: all dist check clean lab$(LAB)
