#include "EmptyBlockElimination.h"
#include <iostream>

using namespace ir;

class TerminatorVisitor : public Visitor {
  public:
    TerminatorVisitor(BasicBlock* source, std::unordered_map<BasicBlock*, std::vector<BasicBlock*>>& m_dependanceMap) :
        source(source), m_dependanceMap(m_dependanceMap) {}

    void visit(BasicJump& jump) { m_dependanceMap[jump.target()].push_back(source); }

    void visit(ConditionalJump& jump) {
        m_dependanceMap[jump.trueTarget()].push_back(source);
        m_dependanceMap[jump.falseTarget()].push_back(source);
    }

  private:
    BasicBlock* source;
    std::unordered_map<BasicBlock*, std::vector<BasicBlock*>>& m_dependanceMap;
};

void EmptyBlockEliminationVisitor::visit(ir::Function& function) {
    m_dependanceMap.clear();
    for (auto& block : function.blocks()) {
        if (block->terminator()) {
            TerminatorVisitor{block.get(), m_dependanceMap}.visitBase(*block->terminator());
        }
    }

    for (auto& block : function.blocks()) {
        visit(*block);
    }
}
void EmptyBlockEliminationVisitor::visit(BasicBlock& block) {
    if (block.terminator()) {
        m_currentBlock = &block;
        visitBase(*block.terminator());
    }
}

void EmptyBlockEliminationVisitor::visit(BasicJump& jump) {
    if (needSkip(m_currentBlock) || m_dependanceMap[m_currentBlock].empty()) {
        return;
    }

    BasicBlock* target = jump.target();
    if (target->terminator()) {
        BasicJump* nextJump = dynamic_cast<BasicJump*>(target->terminator().get());
        if (nextJump) {
            if (target->instructions().empty()) {
                jump.setTarget(nextJump->target());
                setNeedSkip(target);
            } else if (m_dependanceMap.at(target).size() <= 1) {
                // We are the only block referencing this block so we can merge them
                auto& instr = m_currentBlock->instructions();
                std::cerr << instr.size() << " ";
                auto& oldInstr = target->instructions();
                std::move(std::begin(oldInstr), std::end(oldInstr), std::back_inserter(instr));
                std::cerr << oldInstr.size() << std::endl;
                for (auto& i : instr) {
                    std::cerr << *i << std::endl;
                }
                oldInstr.clear();
                jump.setTarget(nextJump->target());
                setNeedSkip(target);
            }
        }
    }
    if (target->instructions().empty() && target->terminator()) {}
}

void EmptyBlockEliminationVisitor::visit(ConditionalJump& jump) {
    if (needSkip(m_currentBlock) || m_dependanceMap[m_currentBlock].empty()) {
        return;
    }

    BasicBlock* trueTarget = jump.trueTarget();
    if (trueTarget->instructions().empty() && trueTarget->terminator()) {
        BasicJump* nextJump = dynamic_cast<BasicJump*>(trueTarget->terminator().get());
        if (nextJump) {
            jump.setTrueTarget(nextJump->target());
            setNeedSkip(trueTarget);
            m_changed = true;
        }
    }

    BasicBlock* falseTarget = jump.falseTarget();
    if (falseTarget->instructions().empty() && trueTarget->terminator()) {
        BasicJump* nextJump = dynamic_cast<BasicJump*>(falseTarget->terminator().get());
        if (nextJump) {
            jump.setFalseTarget(nextJump->target());
            setNeedSkip(falseTarget);
            m_changed = true;
        }
    }
}
