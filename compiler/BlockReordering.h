#pragma once
#include "ir/BasicBlock.h"
#include "ir/Ir.h"
#include "ir/Visitor.h"
#include <algorithm>
#include <deque>
#include <vector>

class BlockReorderingVisitor : public ir::Visitor {
  public:
    using ir::Visitor::visit;

    void visit(ir::Function& function) override;
    void visit(ir::ConditionalJump& jump) override;
    void visit(ir::BasicJump& jump) override;

    bool changed() const { return m_changed; }

  private:
    ir::Function* m_currentFunction;
    std::vector<std::unique_ptr<ir::BasicBlock>> m_finalBlocks;
    std::deque<std::unique_ptr<ir::BasicBlock>> m_toVisit;
    bool m_changed = false;

    void visitNext(ir::BasicBlock* block) {
        auto nextBlockIt = std::find_if(m_currentFunction->blocks().begin(), m_currentFunction->blocks().end(), [block](auto& other) {
            return other.get() == block;
        });

        if (nextBlockIt != m_currentFunction->blocks().end()) 
            m_toVisit.push_back(std::move(*nextBlockIt));
    }
};
