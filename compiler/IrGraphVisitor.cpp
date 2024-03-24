#include "IrGraphVisitor.h"
#include <algorithm>

using namespace std;

void IrGraphVisitor::visit(ir::Function& function) {
    m_out << "digraph cfg {" << endl;
    m_out << "node [shape = \"box\"]" << endl;
    visit(*function.prologue());
    for (const auto& block : function.blocks()) {
        visit(*block);
    }
    visit(*function.epilogue());
    m_out << "}" << endl;
}

void IrGraphVisitor::visit(ir::BasicBlock& block) {
    m_currentBlockLabel = block.label();
    std::ranges::replace(m_currentBlockLabel, '.', '_');
    m_out << m_currentBlockLabel << "[label = \"" << block.label() << ":\\l";
    for (const auto& instr : block.instructions()) {
        m_out << *instr << "\\l";
    }
    m_out << "\"]" << endl;

    if (block.terminator()) {
        visitBase(*block.terminator());
    }
}

void IrGraphVisitor::visit(ir::BasicJump& jump) {
    string targetLabel = jump.target()->label();
    std::ranges::replace(targetLabel, '.', '_');
    m_out << m_currentBlockLabel << " -> " << targetLabel << endl;
}
void IrGraphVisitor::visit(ir::ConditionalJump& jump) {
    string trueTargetLabel = jump.trueTarget()->label();
    std::ranges::replace(trueTargetLabel, '.', '_');
    string falseTargetLabel = jump.falseTarget()->label();
    std::ranges::replace(falseTargetLabel, '.', '_');
    m_out << m_currentBlockLabel << " -> " << trueTargetLabel << " [label = \"" << jump.condition() << "\"]" << endl;
    m_out << m_currentBlockLabel << " -> " << falseTargetLabel << " [label = \"not " << jump.condition() << "\"]" << endl;
}
