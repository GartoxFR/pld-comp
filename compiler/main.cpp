#include <iostream>
#include <fstream>
#include <optional>
#include <sstream>
#include <cstdlib>
#include <variant>

#include "BlockReordering.h"
#include "ConstantFolding.h"
#include "DeadCodeElimination.h"
#include "EmptyBlockElimination.h"
#include "IrGenVisitor.h"
#include "IrPrintVisitor.h"
#include "IrGraphVisitor.h"
#include "IrValuePropagationVisitor.h"
#include "LocalRenaming.h"
#include "X86GenVisitor.h"
#include "antlr4-runtime.h"
#include "generated/ifccLexer.h"
#include "generated/ifccParser.h"
#include "generated/ifccBaseVisitor.h"
#include "ir/Ir.h"

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

    IrGenVisitor visitor;
    visitor.visit(tree);

    if (visitor.hasErrors()) {

        cerr << "Des erreurs sont survenus. Abandon." << endl;
        return 1;
    }

    IrPrintVisitor printer(cerr);
    X86GenVisitor gen(cout);
    for (auto& function : visitor.functions()) {
        ofstream file(function->name() + ".dot");
        IrGraphVisitor cfg(file);
        bool changed;
        do {
            IrValuePropagationVisitor propagator;
            DeadCodeElimination deadCodeElimination;
            ConstantFoldingVisitor folding;
            EmptyBlockEliminationVisitor emptyBlockElimination;
            BlockReorderingVisitor blockReordering;

            propagator.visit(*function);
            deadCodeElimination.visit(*function);
            folding.visit(*function);
            emptyBlockElimination.visit(*function);
            blockReordering.visit(*function);

            changed = propagator.changed() || deadCodeElimination.changed() || folding.changed() ||
                emptyBlockElimination.changed() || blockReordering.changed();
        } while (changed);

        LocalRenamingVisitor localRenaming;
        // localRenaming.visit(*function);

        printer.visit(*function);
        cfg.visit(*function);
        gen.visit(*function);
    }

    return 0;
}
