#include "EmptyBlockElimination.h"

using namespace ir;

void EmptyBlockEliminationVisitor::visit(BasicBlock& block) {
    if (block.terminator()) {
        visitBase(*block.terminator());
    }
}

void EmptyBlockEliminationVisitor::visit(BasicJump& jump) {
    BasicBlock* target = jump.target();
    if (target->instructions().empty() && target->terminator()) {
        BasicJump* nextJump = dynamic_cast<BasicJump*>(target->terminator().get());
        if (nextJump) {
            jump.setTarget(nextJump->target());
        }
    }
}

void EmptyBlockEliminationVisitor::visit(ConditionalJump& jump) {
    BasicBlock* trueTarget = jump.trueTarget();
    if (trueTarget->instructions().empty() && trueTarget->terminator()) {
        BasicJump* nextJump = dynamic_cast<BasicJump*>(trueTarget->terminator().get());
        if (nextJump) {
            jump.setTrueTarget(nextJump->target());
            m_changed = true;
        }
    }

    BasicBlock* falseTarget = jump.falseTarget();
    if (falseTarget->instructions().empty() && trueTarget->terminator()) {
        BasicJump* nextJump = dynamic_cast<BasicJump*>(falseTarget->terminator().get());
        if (nextJump) {
            jump.setFalseTarget(nextJump->target());
            m_changed = true;
        }
    }
}
