# src/t/Makefile
# File ID: 94134e6c-87f2-11ef-8087-83850402c3ce

.PHONY: all
all: test

.PHONY: build
build:
	cd .. && $(MAKE)

.PHONY: clean
clean:
	rm -f gmon.out gmon.sum
	rm -rf tmp-* *-stderr.tmp

.PHONY: test
test: build
	./geocalc.t

.PHONY: valgrind
valgrind: build
	./geocalc.t --valgrind
