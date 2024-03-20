#pragma once
#include "BlockDependance.h"
#include "ir/Ir.h"
#include "ir/Visitor.h"

#include <algorithm>

class EmptyBlockEliminationVisitor : public ir::Visitor {
  public:
    using ir::Visitor::visit;

    EmptyBlockEliminationVisitor(DependanceMap& dependanceMap) : m_dependanceMap(dependanceMap) {}

    void visit(ir::Function& function) override;
    void visit(ir::BasicBlock& block) override;
    void visit(ir::ConditionalJump& jump) override;
    void visit(ir::BasicJump& jump) override;

    bool changed() const { return m_changed; }

  private:
    bool m_changed = false;
    ir::BasicBlock* m_currentBlock;
    DependanceMap& m_dependanceMap;
    std::vector<ir::BasicBlock*> m_toSkip;


    bool needSkip(ir::BasicBlock* block) {
        return std::find(std::begin(m_toSkip), std::end(m_toSkip), block) != std::end(m_toSkip);
    }

    void setNeedSkip(ir::BasicBlock* block) { m_toSkip.push_back(block); }
};
