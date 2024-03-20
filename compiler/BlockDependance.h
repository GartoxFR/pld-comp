#pragma once

#include "ir/Ir.h"
#include "ir/Visitor.h"

using DependanceMap = std::unordered_map<const ir::BasicBlock*, std::vector<ir::BasicBlock*>>;

class DependanceMapVisitor : public ir::Visitor {
  public:
    using ir::Visitor::visit;

    void visit(ir::BasicBlock& block) {
        m_currentBlock = &block;
        if (block.terminator()) {
            visitBase(*block.terminator());
        }
    }
    void visit(ir::BasicJump& jump) { m_dependanceMap[jump.target()].push_back(m_currentBlock); }

    void visit(ir::ConditionalJump& jump) {
        m_dependanceMap[jump.trueTarget()].push_back(m_currentBlock);
        m_dependanceMap[jump.falseTarget()].push_back(m_currentBlock);
    }

    auto& dependanceMap() { return m_dependanceMap; }

  private:
    ir::BasicBlock* m_currentBlock;
    DependanceMap m_dependanceMap;
};

inline DependanceMap computeDependanceMap(ir::Function& function) {
    DependanceMapVisitor visitor;
    visitor.visit(function);

    auto map = std::move(visitor.dependanceMap());
    return map;
}
