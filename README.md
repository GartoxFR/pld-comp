# Structure of the project

```
├── compiler -> Source code of the compiler
│   └── ir   -> Source used to represent the IR
├── doc      -> UML Diagrams
└── tests    -> Tests
```

# Build requirements

- gcc 11.4 or higher
- antlr4.13.1 (install with ./install-antlr.sh)

If you installed antlr with the script, you can then use the following config.mk
```Makefile
ANTLRJAR=../antlr/jar/antlr-4.13.1-complete.jar
ANTLRINC=../antlr/include
ANTLRLIB=../antlr/lib/libantlr4-runtime.a
ANTLR=java -jar $(ANTLRJAR)
```

# Using the root Makefile

There is a Makefile at the root of the project to execute common task faster :

```bash
make -j         # Build the project in parallel
make test -j    # Build the project and execute the tests
```

# Options available
-O0 to get rid of all optimizations done by the compiler 


More technical details are available in the slides provided in the PDF
