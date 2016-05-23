# UCLA CS 111 - Spring '16
# Rahul Malavalli - 204429252
# Arjun Arjun     - 504078752
# Lab 3A - File System Dump

UID = 204429252
LAB = 3a

CC = gcc
CFLAGS = -O2 -Wall -Wextra -Wno-unused -Werror -lrt
LDLIBS = -lm
LDFLAGS = -pthread

DISTDIR = lab$(LAB)-$(UID)

all: lab$(LAB)

SRC = lab$(LAB).c
OBJ = $(subst .c,.o,$(SRC))

DIST_SOURCES = Makefile README $(SRC) lab$(LAB).h

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

run: 	all
	./lab3a --verbose ../disk-image
super:
	sort -n super.csv > my_super.csv
	sort -n ../lab3a-official_csv/correct/super.csv | diff my_super.csv - | cat -n
group:
	sort -n group.csv > my_group.csv
	sort -n ../lab3a-official_csv/correct/group.csv | diff my_group.csv - | cat -n
bitmap:
	sort bitmap.csv > mbitmap.csv
	sort ../lab3a-official_csv/correct/bitmap.csv | diff mbitmap.csv - | cat -n
inode:
	sort -n inode.csv > minode.csv
	sort -n ../lab3a-official_csv/correct/inode.csv | diff minode.csv - | cat -n
directory:
	sort -n directory.csv > my_directory.csv
	sort -n ../lab3a-official_csv/correct/directory.csv | diff my_directory.csv - | cat -n
indirect:
	sort -n indirect.csv > my_indirect.csv
	sort -n ../lab3a-official_csv/correct/indirect.csv | diff my_indirect.csv - | cat -n

check: run super group bitmap inode directory indirect

.PHONY: all dist check clean