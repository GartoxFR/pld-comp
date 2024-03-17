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

# IR

## Creating the ir representation of a function by hand

```c++
using namespace ir;

// Declare a funtion named "main" with no arguments
Function function{"main", 0};

// Declare variables
Local foo = function.newLocal("foo");
Local bar = function.newLocal("bar");

// Declare temporaries
Local temp1 = function.newLocal();
Local temp2 = function.newLocal();

// Allocate the blocks
BasicBlock* block = function.newBlock();
BasicBlock* epilogue = function.newBlock();

// Emit the instructions. This will be done in a visitor as we did
// for x86 ASM
block->emit<Copy>(foo, Immediate(5));
block->emit<Copy>(bar, foo);
block->emit<BinaryOp>(temp1, foo, Immediate(3), BinaryOpKind::MUL);
block->emit<Copy>(temp2, Immediate(1));
block->emit<BinaryOp>(bar, bar, temp2, BinaryOpKind::ADD);

// Place the terminator to end the block. This means that at the end of the block
// we need to jump unconditionally to the epilogue block
block->terminate<BasicJump>(epilogue);

// Print some informations
function.printLocalMapping(std::cout);
cout << block->label() << ":" << endl;
for (const auto& instruction : block->instructions()) {
    cout << "    " << *instruction << endl;
}
cout << "    " << *block->terminator() << endl;
cout << epilogue->label() << ":" << endl;
```

The above snippet prints the folowing :

```
debug {
    _1 => foo
    _2 => bar
}
.main.BB0:
    _1 := 5
    _2 := _1
    _3 := _1 * 3
    _4 := 1
    _2 := _2 + _4
    jump .main.BB1
.main.BB1:
```
