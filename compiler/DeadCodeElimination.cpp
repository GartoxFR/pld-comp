#include "DeadCodeElimination.h"
#include <algorithm>
#include <ranges>

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

void BlockLivenessAnalysis::visit(ir::Function& function) {
    m_dependanceMap.clear();
    for (auto& block : function.blocks()) {
        if (block->terminator()) {
            TerminatorVisitor{block.get(), m_dependanceMap}.visitBase(*block->terminator());
        }
    }

    std::vector<BasicBlock*> toVisit = {function.epilogue()};
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

void BlockLivenessAnalysis::visit(ir::BasicBlock& block) {
    if (block.terminator()) {
        visitBase(*block.terminator());
    }

    for (auto& instr : block.instructions() | std::views::reverse) {
        visitBase(*instr);
    }
}

void BlockLivenessAnalysis::visit(ir::BinaryOp& binaryOp) {
    setLive(binaryOp.left());
    setLive(binaryOp.right());
    unsetLive(binaryOp.destination());
}
void BlockLivenessAnalysis::visit(ir::UnaryOp& unaryOp) {
    setLive(unaryOp.operand());
    unsetLive(unaryOp.destination());
}
void BlockLivenessAnalysis::visit(ir::Assignment& assignment) {
    setLive(assignment.source());
    unsetLive(assignment.destination());
}

void BlockLivenessAnalysis::visit(ir::ConditionalJump& jump) {
    setLive(jump.condition());
}
void BlockLivenessAnalysis::visit(ir::Call& call) {
    for (auto& arg : call.args()) {
        setLive(arg);
    }
}

void DeadCodeElimination::visit(ir::Function& function) {
    BlockLivenessAnalysis analysis;
    analysis.visit(function);
    m_blocksOutputs = analysis.blocksOutputLiveness();

    visit(*function.prologue());
    for (auto& block : function.blocks()) {
        visit(*block);
    }
    visit(*function.epilogue());
}

void DeadCodeElimination::visit(ir::BasicBlock& block) {
    m_workingSet = m_blocksOutputs[&block];

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

void DeadCodeElimination::visit(ir::ConditionalJump& jump) {
    setLive(jump.condition());
}
void DeadCodeElimination::visit(ir::Call& call) {
    for (auto& arg : call.args()) {
        setLive(arg);
    }
}
