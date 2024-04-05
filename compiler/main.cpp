#include <iostream>
#include <fstream>
#include <optional>
#include <sstream>
#include <cstdlib>
#include <string_view>
#include <variant>

#include "BlockDependance.h"
#include "BlockLivenessAnalysis.h"
#include "BlockReordering.h"
#include "ConstantFolding.h"
#include "DeadCodeElimination.h"
#include "EmptyBlockElimination.h"
#include "IrGenVisitor.h"
#include "IrPrintVisitor.h"
#include "IrGraphVisitor.h"
#include "IrValuePropagationVisitor.h"
#include "BlockDependance.h"
#include "LocalRenaming.h"
#include "PointedLocalGatherer.h"
#include "TwoStepAssignmentElimination.h"
#include "Type.h"
#include "X86GenVisitor.h"
#include "antlr4-runtime.h"
#include "generated/ifccLexer.h"
#include "generated/ifccParser.h"
#include "generated/ifccBaseVisitor.h"
#include "ir/Ir.h"

using namespace antlr4;
using namespace std;

static void performOptimizations(ir::Function& function) {
        bool changed;
        do {
            PointedLocals pointedLocals = computePointedLocals(function);

            IrValuePropagationVisitor propagator{pointedLocals};
            propagator.visit(function);

            DependanceMap dependanceMap = computeDependanceMap(function);
            BlockLivenessAnalysis livenessAnalysis = computeBlockLivenessAnalysis(function, dependanceMap);

            DeadCodeElimination deadCodeElimination{livenessAnalysis, pointedLocals};
            deadCodeElimination.visit(function);

            ConstantFoldingVisitor folding;
            folding.visit(function);

            livenessAnalysis = computeBlockLivenessAnalysis(function, dependanceMap);
            TwoStepAssignmentEliminationVisitor elimination{livenessAnalysis, pointedLocals};
            elimination.visit(function);

            // Recompute dependance map as it may have changed
            dependanceMap = computeDependanceMap(function);

            EmptyBlockEliminationVisitor emptyBlockElimination{dependanceMap};
            emptyBlockElimination.visit(function);

            BlockReorderingVisitor blockReordering;
            blockReordering.visit(function);

            changed = propagator.changed() || deadCodeElimination.changed() || folding.changed() ||
                emptyBlockElimination.changed() || blockReordering.changed() || elimination.changed();
        } while (changed);
}

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

    bool optimize = true;
    for (int i = 1; i < argn; i++) {
        std::string_view arg = argv[i];
        if (arg == "-O0") {
            optimize = false;
        }
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
    globalTypePool.init();

    IrGenVisitor visitor;
    visitor.visit(tree);

    if (visitor.hasErrors()) {

        cerr << "Des erreurs sont survenues. Abandon." << endl;
        return 1;
    }

    IrPrintVisitor printer(cerr);
    X86GenVisitor gen(cout, optimize);
    for (auto& function : visitor.functions()) {
        ofstream file(function->name() + ".dot");
        IrGraphVisitor cfg(file);

        if (optimize) {
            performOptimizations(*function);
        }

        LocalRenamingVisitor localRenaming;
        localRenaming.visit(*function);

        printer.visit(*function);
        cfg.visit(*function);
        gen.visit(*function);
    }

    return 0;
}
