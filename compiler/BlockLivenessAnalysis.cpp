#include "BlockLivenessAnalysis.h"
#include <algorithm>
#include <ranges>
#include <variant>

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
    m_liveMap[function.epilogue()].second = {function.returnLocal()};
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
    unsetLive(binaryOp.destination());
    setLive(binaryOp.left());
    setLive(binaryOp.right());
}
void BlockLivenessAnalysisVisitor::visit(ir::UnaryOp& unaryOp) {
    unsetLive(unaryOp.destination());
    setLive(unaryOp.operand());
}
void BlockLivenessAnalysisVisitor::visit(ir::Assignment& assignment) {
    unsetLive(assignment.destination());
    setLive(assignment.source());
}

void BlockLivenessAnalysisVisitor::visit(ir::Cast& cast) {
    unsetLive(cast.destination());
    setLive(cast.source());
}

void BlockLivenessAnalysisVisitor::visit(ir::ConditionalJump& jump) { setLive(jump.condition()); }
void BlockLivenessAnalysisVisitor::visit(ir::Call& call) {
    if (m_calls) {
        (*m_calls)[&call].second = m_workingSet;
    }
    unsetLive(call.destination());
    for (auto& arg : call.args()) {
        setLive(arg);
    }
    if (m_calls) {
        (*m_calls)[&call].first = m_workingSet;
    }
}

void BlockLivenessAnalysisVisitor::visit(ir::PointerRead& read) {
    unsetLive(read.destination());
    setLive(read.address());
}
void BlockLivenessAnalysisVisitor::visit(ir::PointerWrite& write) {
    setLive(write.address());
    setLive(write.source());
}
void BlockLivenessAnalysisVisitor::visit(ir::AddressOf& address) {
    unsetLive(address.destination());
    if (std::holds_alternative<ir::Local>(address.source())) {
        setLive(std::get<ir::Local>(address.source()));
    }
}
