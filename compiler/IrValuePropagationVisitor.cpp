#include "IrValuePropagationVisitor.h"
#include <algorithm>
#include <iterator>

using namespace ir;
;

void GlobalValuePropagationVisitor::visit(ir::Function& function) {
    m_toVisit.clear();
    m_toVisit.push_back(function.epilogue());
    std::transform(
        std::rbegin(function.blocks()), std::rend(function.blocks()), std::back_inserter(m_toVisit),
        [](auto& block) { return block.get(); }
    );
    m_toVisit.push_back(function.prologue());

    while (!m_toVisit.empty()) {
        auto current = m_toVisit.back();
        m_workingMapping = m_localMappings[current].first;
        m_toVisit.pop_back();
        m_currentBlock = current;
        visit(*current);
    }
}

void GlobalValuePropagationVisitor::visit(ir::BinaryOp& binaryOp) { setNotConstant(binaryOp.destination()); }
void GlobalValuePropagationVisitor::visit(ir::UnaryOp& unaryOp) { setNotConstant(unaryOp.destination()); }
void GlobalValuePropagationVisitor::visit(ir::Assignment& assignment) {
    setMapping(assignment.destination(), assignment.source());
}
void GlobalValuePropagationVisitor::visit(ir::Call& call) { setNotConstant(call.destination()); }

void GlobalValuePropagationVisitor::visit(ir::BasicJump& jump) {
    if (flushBlockOutput()) {
        propagate(jump.target());
    }
}
void GlobalValuePropagationVisitor::visit(ir::ConditionalJump& jump) {
    if (flushBlockOutput()) {
        propagate(jump.trueTarget());
        propagate(jump.falseTarget());
    }
}

void IrValuePropagationVisitor::visit(ir::Function& function) {
    GlobalValuePropagationVisitor globalValuePropagationVisitor;
    globalValuePropagationVisitor.visit(function);
    m_earlyMappings = globalValuePropagationVisitor.mappings();

    visit(*function.prologue());
    for (auto& block : function.blocks()) {
        visit(*block);
    }
    visit(*function.epilogue());
}
void IrValuePropagationVisitor::visit(ir::BasicBlock& block) {
    m_knownValues = m_earlyMappings[&block];
    for (auto& instr : block.instructions()) {
        visitBase(*instr);
    }

    if (block.terminator()) {
        visitBase(*block.terminator());
    }
}
void IrValuePropagationVisitor::visit(ir::BinaryOp& binaryOp) {
    trySubstitute(binaryOp.left());
    trySubstitute(binaryOp.right());
    invalidateLocal(binaryOp.destination());
}
void IrValuePropagationVisitor::visit(ir::UnaryOp& unaryOp) {
    trySubstitute(unaryOp.operand());
    invalidateLocal(unaryOp.destination());
}
void IrValuePropagationVisitor::visit(ir::Assignment& assignment) {
    trySubstitute(assignment.source());
    setSubstitution(assignment.destination(), assignment.source());
}

void IrValuePropagationVisitor::visit(ir::ConditionalJump& jump) { trySubstitute(jump.condition()); }
void IrValuePropagationVisitor::visit(ir::Call& call) {
    for (auto& arg : call.args()) {
        trySubstitute(arg);
    }
}
