#include "BlockLivenessAnalysis.h"
#include <algorithm>
#include <ranges>

using namespace ir;

void BlockLivenessAnalysisVisitor::visit(ir::Function& function) {
    std::vector<BasicBlock*> toVisit;
    // Push all block to be sure they are all visited at least once
    toVisit.push_back(function.prologue());
    std::transform(
        std::begin(function.blocks()), std::end(function.blocks()), std::back_inserter(toVisit),
        [](auto& block) { return block.get(); }
    );
    toVisit.push_back(function.epilogue());
    m_liveMap[function.epilogue()].second = {Local{0}};
    while (!toVisit.empty()) {
        BasicBlock* current = toVisit.back();
        toVisit.pop_back();
        m_workingSet = m_liveMap[current].second;

        visit(*current);

        if (flushBlockInput(current)) {
            for (auto dep : m_dependanceMap[current]) {
                if (propagate(current, dep)) {
                    toVisit.push_back(dep);
                }
            }
        }
    }
}

void BlockLivenessAnalysisVisitor::visit(ir::BasicBlock& block) {
    if (block.terminator()) {
        visitBase(*block.terminator());
    }

    for (auto& instr : block.instructions() | std::views::reverse) {
        visitBase(*instr);
    }
}

void BlockLivenessAnalysisVisitor::visit(ir::BinaryOp& binaryOp) {
    setLive(binaryOp.left());
    setLive(binaryOp.right());
    unsetLive(binaryOp.destination());
}
void BlockLivenessAnalysisVisitor::visit(ir::UnaryOp& unaryOp) {
    setLive(unaryOp.operand());
    unsetLive(unaryOp.destination());
}
void BlockLivenessAnalysisVisitor::visit(ir::Assignment& assignment) {
    setLive(assignment.source());
    unsetLive(assignment.destination());
}

void BlockLivenessAnalysisVisitor::visit(ir::ConditionalJump& jump) { setLive(jump.condition()); }
void BlockLivenessAnalysisVisitor::visit(ir::Call& call) {
    for (auto& arg : call.args()) {
        setLive(arg);
    }
}

