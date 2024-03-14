#include "IrPrintVisitor.h"

using namespace std;

void IrPrintVisitor::visit(ir::Function& function) {
    function.printLocalMapping(m_out);
    visit(*function.prologue());
    for (const auto& block : function.blocks()) {
        visit(*block);
    }
    visit(*function.epilogue());
}

void IrPrintVisitor::visit(ir::BasicBlock& block) {
    m_out << block.label() << endl;
    for (const auto& instr : block.instructions()) {
        m_out << "    " << *instr << endl;
    }

    if (block.terminator()) {
        m_out << "    " << *block.terminator() << endl;
    }
}
