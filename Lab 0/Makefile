# CS 111 Lab1 Makefile
# Project-0 : Spring 2016
# Arjun 504078752

CC = gcc
CFLAGS = -g -Wall -O2 -Wextra -Wno-unused -Werror
LAB = 0
DISTDIR = lab$(LAB)-arjun

G=\x1b[32;01m
R=\x1b[31;01m
W=\x1b[0m
PASS=$(G)[PASSED]$(W)
FAIL=$(R)[FAILED]$(W)

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
CHECK_1 = [ -f random.blk.tmp ]
CHECK_2 = diff random.blk random.blk.tmp 2>/dev/null

CMD_2   = ./lab$(LAB) --output random.blk.tmp --input random.blk
CHECK_3 = [ -f random.blk.tmp ]
CHECK_4 = diff random.blk random.blk.tmp 2>/dev/null

CMD_3   = ./lab$(LAB) --segfault
CHECK_5 = echo $?

CMD_4   = ./lab$(LAB) --catch --segfault
CHECK_6 = echo $?

check: all
	@echo -e "\t\x1b[1;01m Generate files for test :"
	dd if=/dev/urandom of=random.blk bs=4096 count=1
	@echo -e "\x1b[0m"

	@echo -e "\x1b[34;01m COMMAND :: $(CMD_1) \x1b[0m" && $(CMD_1)
	@echo -en "\t\x1b[1;01m [CHECK 1/2] \x1b[0m :: $(CHECK_1) :: "
	@$(CHECK_1) && echo -e "$(PASS)" || echo -e "$(FAIL)"
	@echo -en "\t\x1b[1;01m [CHECK 2/2] \x1b[0m :: $(CHECK_2) :: "
	@$(CHECK_2) && echo -e "$(PASS)" || echo -e "$(FAIL)"
	@rm random.blk.tmp

	@echo -e "\x1b[34;01m COMMAND :: $(CMD_2) \x1b[0m" && $(CMD_2)
	@echo -en "\t\x1b[1;01m [CHECK 1/2] \x1b[0m :: $(CHECK_3) :: "
	@$(CHECK_3) && echo -e "$(PASS)" || echo -e "$(FAIL)"
	@echo -en "\t\x1b[1;01m [CHECK 2/2] \x1b[0m :: $(CHECK_4) :: "
	@$(CHECK_4) && echo -e "$(PASS)" || echo -e "$(FAIL)"

	@echo -e "\x1b[34;01m COMMAND :: $(CMD_3) \x1b[0m" && ${CMD_3}
	@echo -en "\t\x1b[1;01m [CHECK 1/1] \x1b[0m :: $(CHECK_5) :: "
	@$(CHECK_5) && echo -e "$(PASS)" || echo -e "$(FAIL)"

	@echo -e "\x1b[34;01m COMMAND :: $(CMD_4) \x1b[0m" && $(CMD_4)
	@echo -en "\t\x1b[1;01m [CHECK 1/1] \x1b[0m :: $(CHECK_6) :: "
	@$(CHECK_6) && echo -e "$(PASS)" || echo -e "$(FAIL)"

	@echo -n "Cleaning up temporary files..."
	@rm random.blk* && echo "done."

clean:
	rm -rf *.o *~ *.bak *.tar.gz core *.core *.tmp lab$(LAB) $(DISTDIR)

.PHONY: all dist check clean
