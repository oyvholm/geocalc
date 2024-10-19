# Makefile for geocalc.c
# File ID: 93f52a4a-87f2-11ef-bbc3-83850402c3ce
# Author: Øyvind A. Holm <sunny@sunbase.org>

PREFIX = /usr/local

EXEC = geocalc

CC = cc
LD = cc

CFLAGS  =
CFLAGS += $$(test "$$(uname -o)" = "Android" || echo -fcf-protection=full)
CFLAGS += $$(test -n "$(GCOV)" && echo -n "-fprofile-arcs -ftest-coverage")
CFLAGS += $$(test -n "$(PROF)" && echo -n "-pg")
CFLAGS += -O0
CFLAGS += -Wall
CFLAGS += -Wcast-align
CFLAGS += -Wcast-qual
CFLAGS += -Wconversion
CFLAGS += -Wdouble-promotion
CFLAGS += -Werror
CFLAGS += -Werror=implicit-function-declaration
CFLAGS += -Wextra
CFLAGS += -Wfloat-conversion
CFLAGS += -Wformat
CFLAGS += -Wformat-overflow
CFLAGS += -Wformat-security
CFLAGS += -Wformat-truncation
CFLAGS += -Wpedantic
CFLAGS += -Wshadow
CFLAGS += -Wstrict-aliasing
CFLAGS += -Wstrict-overflow=5
CFLAGS += -Wstrict-prototypes
CFLAGS += -c
CFLAGS += -fno-common
CFLAGS += -fstack-protector-strong
CFLAGS += -fwrapv
CFLAGS += -g3
CFLAGS += -pedantic
CFLAGS += -std=c99

LDFLAGS  =
LDFLAGS += $$(test -n "$(PROF)" && echo -n "-pg")
LDFLAGS += -Wl,-z,now
LDFLAGS += -Wl,-z,relro

LIBS  =
LIBS += $$(test -n "$(GCOV)" && echo "-lgcov --coverage")

OBJS  =
OBJS += geocalc.o
OBJS += selftest.o

CFILES  =
CFILES += geocalc.c
CFILES += selftest.c

HFILES  =
HFILES += geocalc.h

DEPS = version.h $(HFILES) Makefile
GNCOV_STR = $$(test -n "$(GNCOV)" && echo "-g")
TESTABLE_LINES = $$(grep -E '^ +\#\#\#\#\#:' *.c.gcov \
                    | grep -vF '/* gncov */' | wc -l)

.PHONY: all
all: $(EXEC)

$(EXEC): $(OBJS)
	$(LD) -o $(EXEC) $(LDFLAGS) $(OBJS) $(LIBS)

version.h: Gen-version $(CFILES) $(HFILES) Makefile
	./Gen-version

geocalc.o: geocalc.c $(DEPS)
	$(CC) $(CFLAGS) geocalc.c

selftest.o: selftest.c $(DEPS)
	$(CC) $(CFLAGS) selftest.c

.PHONY: clean
clean:
	rm -f $(EXEC) $(OBJS)
	rm -f *.gcda *.gcno *.gcov
	rm -f gmon.out gmon.sum
	rm -f version.h
	cd t && $(MAKE) clean

.PHONY: edit
edit: tags
	$(EDITOR) $$(git ls-files)
	rm tags

.PHONY: gcov
gcov:
	$(MAKE) clean test GCOV=1
	gcov $(CFILES)
	@echo $$(grep -E '^ +#####:' *.c.gcov | wc -l) untested lines >&2
	@echo $$(grep -E '^ +#####:' *.c.gcov | grep -F '/* gncov */' \
	         | wc -l) gncov lines >&2
	@echo $(TESTABLE_LINES) testable lines >&2

.PHONY: gcov-cmt
gcov-cmt: gcov
	gcov-cmt $(GNCOV_STR) $(CFILES)

.PHONY: gcov-cmt-clean
gcov-cmt-clean:
	gcov-cmt -d $(CFILES)

.PHONY: gdb
gdb: $(EXEC)
	gdb -x gdbrc $$(cat gdbopts 2>/dev/null) $(EXEC)

.PHONY: install
install: $(PREFIX)/bin/$(EXEC)

$(PREFIX)/bin/$(EXEC): $(EXEC)
	mkdir -p $(PREFIX)/bin
	install $(EXEC) $(PREFIX)/bin/$(EXEC)

tags: $(CFILES) $(HFILES)
	ctags $(CFILES) $(HFILES)

.PHONY: test
test: $(EXEC)
	cd t && $(MAKE) test

.PHONY: testall
testall:
	$(MAKE) testcomb WHAT=test

.PHONY: testcomb
testcomb:
	$(MAKE) clean $(WHAT) NDEBUG=1
	$(MAKE) clean $(WHAT) UNUSED=1
	$(MAKE) clean $(WHAT) NDEBUG=1 UNUSED=1
	$(MAKE) clean $(WHAT)

.PHONY: testsrc
testsrc:
	@echo Check files for long lines >&2
	@for f in $(CFILES) $(HFILES) Gen-version Makefile; do \
		[ -f "$$f" ] && expand "$$f" | sed 's/ $$//;' \
		| grep -q -E '.{80}' && echo "$$f"; \
	done | grep . && exit 1 || true

.PHONY: tlok
tlok:
	@$(MAKE) -s gcov 2>.make-tlok.tmp >&2; res=$$?; \
	if test $$res -eq 0; then \
		printf '%u:%u\n' \
		       "$(TESTABLE_LINES)" \
		       "$$(grep ^1\\.\\. .make-tlok.tmp \
		           | perl -pe 's/^1\.\.(\d+).*/$$1/;')"; \
	else \
		grep "Looks like you failed" <.make-tlok.tmp \
		| perl -p \
		  -e  's/.*failed (\d+) tests? of (\d+).*/$$1\/$$2 FAIL/;'; \
	fi; \
	rm .make-tlok.tmp

.PHONY: uninstall
uninstall:
	rm -f $(PREFIX)/bin/$(EXEC)

.PHONY: valgrind
valgrind:
	cd t && $(MAKE) valgrind

.PHONY: valgrindall
valgrindall:
	$(MAKE) testcomb WHAT=valgrind
