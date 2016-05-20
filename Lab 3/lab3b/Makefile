# UCLA CS 111 - Spring '16
# Rahul Malavalli - 204429252
# Arjun Arjun     - 504078752
# Lab 3B - File System Analysis

UID = 204429252
LAB = 3b

DISTDIR = lab$(LAB)-$(UID)

all: run

SRC = lab$(LAB).py

DIST_SOURCES = Makefile README $(SRC)

dist: $(DISTDIR).tar.gz

$(DISTDIR).tar.gz: $(DIST_SOURCES)
	@rm -rf $(DISTDIR)
	@export GZIP=-9
	@tar -cvzf $@.tmp $(DIST_SOURCES)
	@mv $@.tmp $@
	@echo "tarball created :: $@"

clean:
	rm -rf *.log *.o *~ *.bak *.tar.gz core *.core *.tmp lab3b_check.txt $(DISTDIR)

run: $(SRC)
	python $(SRC) --verbose

check: run lab3b_check.txt sorted_lab3b_check.txt
			sort lab3b_check.txt | diff - sorted_lab3b_check.txt

.PHONY: run all dist check clean
