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

    unsetLive(binaryOp.destination());
    setLive(binaryOp.left());
    setLive(binaryOp.right());
}
void DeadCodeElimination::visit(ir::UnaryOp& unaryOp) {
    if (tryDrop(unaryOp.destination())) {
        return;
    }
    unsetLive(unaryOp.destination());
    setLive(unaryOp.operand());
}
void DeadCodeElimination::visit(ir::Assignment& assignment) {
    if (tryDrop(assignment.destination())) {
        return;
    }

    unsetLive(assignment.destination());
    setLive(assignment.source());
}

void DeadCodeElimination::visit(ir::Cast& cast) {
    if (tryDrop(cast.destination())) {
        return;
    }

    unsetLive(cast.destination());
    setLive(cast.source());
}

void DeadCodeElimination::visit(ir::ConditionalJump& jump) { setLive(jump.condition()); }
void DeadCodeElimination::visit(ir::Call& call) {
    for (auto& arg : call.args()) {
        setLive(arg);
    }
}

void DeadCodeElimination::visit(ir::PointerRead& read) {
    unsetLive(read.destination());
    setLive(read.address());
}
void DeadCodeElimination::visit(ir::PointerWrite& write) {
    setLive(write.address());
    setLive(write.source());
}
void DeadCodeElimination::visit(ir::AddressOf& address) {
    unsetLive(address.destination());
    if (std::holds_alternative<ir::Local>(address.source())) {
        setLive(std::get<ir::Local>(address.source()));
    }
}
