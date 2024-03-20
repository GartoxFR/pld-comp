#include "DeadCodeElimination.h"
#include <algorithm>
#include <iostream>
#include <iterator>
#include <ranges>

void DeadCodeElimination::visit(ir::Function& function) {
    visit(*function.prologue());
    for (auto& block : function.blocks()) {
        visit(*block);
    }
    visit(*function.epilogue());
}

void DeadCodeElimination::visit(ir::BasicBlock& block) {
    m_workingSet = m_blocksLivenessAnalysis[&block].second;

    if (block.terminator()) {
        visitBase(*block.terminator());
    }

    for (auto& instr : block.instructions() | std::views::reverse) {
        m_currentInstruction = &instr;
        visitBase(*instr);
    }

    auto removeIt = std::remove(block.instructions().begin(), block.instructions().end(), nullptr);
    block.instructions().erase(removeIt, block.instructions().end());
}

void DeadCodeElimination::visit(ir::BinaryOp& binaryOp) {
    if (tryDrop(binaryOp.destination())) {
        return;
    }

    setLive(binaryOp.left());
    setLive(binaryOp.right());
    unsetLive(binaryOp.destination());
}
void DeadCodeElimination::visit(ir::UnaryOp& unaryOp) {
    if (tryDrop(unaryOp.destination())) {
        return;
    }
    setLive(unaryOp.operand());
    unsetLive(unaryOp.destination());
}
void DeadCodeElimination::visit(ir::Assignment& assignment) {
    if (tryDrop(assignment.destination())) {
        return;
    }

    setLive(assignment.source());
    unsetLive(assignment.destination());
}

void DeadCodeElimination::visit(ir::ConditionalJump& jump) { setLive(jump.condition()); }
void DeadCodeElimination::visit(ir::Call& call) {
    for (auto& arg : call.args()) {
        setLive(arg);
    }
}
