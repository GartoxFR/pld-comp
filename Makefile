.PHONY: all build test clean

all: build

build:
	$(MAKE) -C compiler

test: build
	@./tests/ifcc-test.py ./tests/testfiles

clean:
	$(MAKE) -C compiler clean
	rm -rf ifcc-test-output
