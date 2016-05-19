# UCLA CS 111 - Spring '16
# Rahul Malavalli - 204429252
# Arjun Arjun     - 504078752
# Lab 3A - File System Dump

UID = 504078752
LAB = 3a

CC = gcc
CFLAGS = -O2 -Wall -Wextra -Wno-unused -Werror -lrt
LDLIBS = -lm
LDFLAGS = -pthread

DISTDIR = lab$(LAB)-$(UID)

all: lab$(LAB)

SRC = lab$(LAB).c
OBJ = $(subst .c,.o,$(SRC))

DIST_SOURCES = Makefile README lab$(LAB).c

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
	rm -rf *.log *.o *~ *.bak *.tar.gz core *.core *.tmp *.csv lab$(LAB) $(DISTDIR)

cbitmap:
	sort bitmap.csv > mbitmap.csv
	sort ../lab3a-official_csv/correct/bitmap.csv > obitmap.csv
	diff mbitmap.csv obitmap.csv | cat -n

cinode: sort inode.csv > minode.csv
	sort ../lab3a-official_csv/correct/inode.csv > oinode.csv
	diff minode.csv oinode.csv | cat -n

.PHONY: all dist check clean
