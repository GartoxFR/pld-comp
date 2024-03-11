# Structure of the project

```
├── compiler -> Source code of the compiler
│   └── ir   -> Source used to represent the IR
├── doc      -> UML Diagrams
└── tests    -> Tests
```

# Using the root Makefile

There is a Makefile at the root of the project to execute common task faster :

```bash
make -j         # Build the project in parallel
make test -j    # Build the project and execute the tests
```

# IR

## Creating a block

```c++
using namespace ir;

// Construct some strings to represent variable names
Ident foo = make_ident("foo"), bar = make_ident("bar");

// Declare the block, usually they will be given to us
// by the ControlFlowGraph but this is fine for an example
BasicBlock block(0);
BasicBlock epilogue(1);

// Emit the instructions. This will be done in a visitor as we did
// for x86 ASM
block.emit<Copy>(Variable(foo), Immediate(5));
block.emit<Copy>(Variable(bar), Variable(foo));
block.emit<BinaryOp>(Temporary(0), Variable(foo), Immediate(3), BinaryOpKind::MUL);
block.emit<BinaryOp>(Variable(bar), Variable(bar), Immediate(1), BinaryOpKind::ADD);

// Place the terminator to end the block. This means that at the end of the block
// we need to jump unconditionally to the epilogue block
block.terminate<BasicJump>(&epilogue);

// Print some informations
cout << block << ":" << endl;
for (const auto& instruction : block.instructions()) {
    cout << "    " << *instruction << endl;
}
cout << "    " << *block.terminator() << endl;
```

The above snippet prints the folowing :

```
BasicBlock(label = BB0, instructionCount = 4, terminated = true):
    foo := 5
    bar := foo
    t0 := foo * 3
    bar := bar + 1
    jump BB1
```
