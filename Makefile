.PHONY: all build test clean

all: build

build:
	$(MAKE) -C compiler

FILE ?= tests/testfiles

test: build
	@./tests/ifcc-test.py $(FILE)

clean:
	$(MAKE) -C compiler clean
	rm -rf ifcc-test-output
