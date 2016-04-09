# CS 111 Lab1 Makefile
# Project-0 : Spring 2016
# Arjun 504078752

CC = gcc
CFLAGS = -g -Wall -O2 -Wextra -Wno-unused -Werror
LAB = 0
DISTDIR = lab$(LAB)-arjun

PASS=$(\x1b[32;01m)[PASSED]$(\x1b[0m)
FAIL=$(\x1b[31;01m)[FAILED]$(\x1b[0m)

all: lab$(LAB)

SCREENSHOTS = $(wildcard screenshot*.png)

SOURCES = main.c \

OBJECTS = $(subst .c,.o,$(SOURCES))

DIST_SOURCES = \
  $(OBJECTS) Makefile \
  $(SCREENSHOTS) README

lab$(LAB): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

dist: $(DISTDIR).tar.gz

$(DISTDIR).tar.gz: $(DIST_SOURCES)
	@rm -rf $(DISTDIR)
	@tar -czf $@.tmp --transform='s,^,$(DISTDIR)/,' $(DIST_SOURCES)
	@mv $@.tmp $@
	@echo "tarball created :: $@"

CMD_1   = ./lab$(LAB) --input random.blk --output random.blk.tmp
CHECK_1 = diff random.blk random.blk.tmp 2>/dev/null

CMD_2   = ./lab$(LAB) --input random.blk --output random.blk.tmp
CHECK_2 = diff random.blk random.blk.tmp 2>/dev/null

CMD_3   = ./lab$(LAB) --input random.blk --output random.blk.tmp
CHECK_3 = diff random.blk random.blk.tmp 2>/dev/null

CMD_4   = ./lab$(LAB) --input random.blk --output random.blk.tmp
CHECK_4 = diff random.blk random.blk.tmp 2>/dev/null

CMD_5   = ./lab$(LAB) --input random.blk --output random.blk.tmp
CHECK_5 = diff random.blk random.blk.tmp 2>/dev/null

check: all

	@dd if=/dev/urandom of=random.blk bs=4096 count=1 2>/dev/null

	@echo "COMMAND :: $(CMD_1)" && $(CMD_1)
	@echo -n "\tCHECK 1/5 :: $(CHECK_1) :: "
	@$(CHECK_1) && echo "$(PASS)" || echo "$(FAIL)"

	@$(CMD_2)  && echo -n "CHECK 2/5 : Output File Integrity : $(CMD_2) :: "
	@$(CHECK_2) && echo "PASSED" || echo "FAILED"

	@$(CMD_3)  && echo -n "CHECK 3/5 : Output File Integrity : $(CMD_3) :: "
	@$(CHECK_3) && echo "PASSED" || echo "FAILED"

	@$(CMD_4)  && echo -n "CHECK 4/5 : Output File Integrity : $(CMD_4) :: "
	@$(CHECK_4) && echo "PASSED" || echo "FAILED"

	@$(CMD_5)  && echo -n "CHECK 5/5 : Output File Integrity : $(CMD_5) :: "
	@$(CHECK_5) && echo "PASSED" || echo "FAILED"

	@echo -n "Cleaning up temporary files..."
	@rm random.blk* && echo "done."

clean:
	rm -rf *.o *~ *.bak *.tar.gz core *.core *.tmp lab$(LAB) $(DISTDIR)

.PHONY: all dist check clean
