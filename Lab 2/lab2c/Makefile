# UCLA CS 111 - Spring '16
# Arjun 504078752
# Lab 2C - Lock Granularity and Performance

UID = 504078752
LAB = 2c

CC = gcc
CFLAGS = -O3 -Wall -Wextra -Wno-unused -Werror -lrt
LDLIBS = -lm
LDFLAGS = -pthread

DISTDIR = lab$(LAB)-$(UID)

all: lab$(LAB)

SRC = lab$(LAB).c
OBJ = $(subst .c,.o,$(SRC))

DIST_SOURCES = Makefile README SortedList.h SortedList.c lab$(LAB).c \
	graph.png

lab$(LAB): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ) $(LDLIBS)

dist: $(DISTDIR).tar.gz

$(DISTDIR).tar.gz: $(DIST_SOURCES)
	@rm -rf $(DISTDIR)
	@export GZIP=-9
	@tar -cvzf $@.tmp $(DIST_SOURCES)
	@mv $@.tmp $@
	@echo "tarball created :: $@"

clean:
	rm -rf *.log *.o *~ *.bak *.tar.gz core *.core *.tmp lab$(LAB) $(DISTDIR)

.PHONY: all dist check clean
