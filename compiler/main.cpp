#include <iostream>
#include <fstream>
#include <optional>
#include <sstream>
#include <cstdlib>
#include <variant>

#include "SymbolTableVisitor.h"
#include "antlr4-runtime.h"
#include "generated/ifccLexer.h"
#include "generated/ifccParser.h"
#include "generated/ifccBaseVisitor.h"

#include "CodeGenVisitor.h"
#include "ir/Instructions.h"
#include "ir/Visitor.h"
#include "ir/Visitable.h"

using namespace antlr4;
using namespace std;

int main(int argn, const char** argv) {
    using namespace ir;

    std::unique_ptr<Copy> i1 = std::make_unique<Copy>(Temporary(3), Immediate(2));

    auto i = ir::visit(
        visitor{
            [&](Copy& copy) -> std::optional<int> { return {}; },
            [&](BinaryOp& op) -> std::optional<int> { return 5; },
        },
        *i1
    );

    cout << i.value_or(0) << endl;
    ir::visit(visitor{[&](Copy& copy) { cout << copy << endl; }}, *i1);

    stringstream in;
    if (argn == 2) {
        ifstream lecture(argv[1]);
        if (!lecture.good()) {
            cerr << "error: cannot read file: " << argv[1] << endl;
            exit(1);
        }
        in << lecture.rdbuf();
    } else {
        cerr << "usage: ifcc path/to/file.c" << endl;
        exit(1);
    }

    ANTLRInputStream input(in.str());

    ifccLexer lexer(&input);
    CommonTokenStream tokens(&lexer);

    tokens.fill();

    ifccParser parser(&tokens);
    tree::ParseTree* tree = parser.axiom();

    if (parser.getNumberOfSyntaxErrors() != 0) {
        cerr << "error: syntax error during parsing" << endl;
        exit(1);
    }

    SymbolTableVisitor symbolTableVisitor;
    if (!symbolTableVisitor.createSymbolTable(tree)) {
        cerr << "Des erreurs sont survenus lors de la création de la table des "
                "symboles. Abandon."
             << endl;
        return 1;
    }

    CodeGenVisitor v(symbolTableVisitor.getSymbolTable());
    v.visit(tree);

    return 0;
}
