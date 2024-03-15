#include <iostream>
#include <fstream>
#include <optional>
#include <sstream>
#include <cstdlib>
#include <variant>

#include "IrGenVisitor.h"
#include "IrPrintVisitor.h"
#include "IrGraphVisitor.h"
#include "SymbolTableVisitor.h"
#include "X86GenVisitor.h"
#include "antlr4-runtime.h"
#include "generated/ifccLexer.h"
#include "generated/ifccParser.h"
#include "generated/ifccBaseVisitor.h"
#include "ir/Ir.h"

#include "CodeGenVisitor.h"

using namespace antlr4;
using namespace std;

int main(int argn, const char** argv) {
    stringstream in;
    if (argn >= 2) {
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

    // Just print ir
    if (argn >= 3 && strcmp(argv[2], "-ir") == 0) {
        IrGenVisitor visitor;
        visitor.visit(tree);

        if (visitor.hasErrors()) {

            cerr << "Des erreurs sont survenus. Abandon." << endl;
            return 1;
        }

        IrPrintVisitor printer(cerr);
        for (auto& function : visitor.functions()) {
            printer.visit(*function);
        }

        cerr << endl << endl;

        ofstream file("graph.dot");

        IrGraphVisitor cfg(file);
        for (auto& function : visitor.functions()) {
            cfg.visit(*function);
        }

        X86GenVisitor gen(cout);
        for (auto& function : visitor.functions()) {
            gen.visit(*function);
        }

        return 0;
    }

    SymbolTableVisitor symbolTableVisitor;

    if (!symbolTableVisitor.createSymbolTable(tree)) {
        cerr << "Des erreurs sont survenus lors de la création de la table des "
                "symboles. Abandon."
             << endl;
        return 1;
    }

    CodeGenVisitor v;
    v.visit(tree);

    return 0;
}
