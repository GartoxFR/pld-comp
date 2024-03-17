#pragma once
#include "ir/Ir.h"
#include "ir/Visitor.h"

class EmptyBlockEliminationVisitor : public ir::Visitor {
  public:
    using ir::Visitor::visit;

    void visit(ir::BasicBlock& block) override;
    void visit(ir::ConditionalJump& jump) override;
    void visit(ir::BasicJump& jump) override;

    bool changed() const { return m_changed; }

  private:
    bool m_changed = false;
};
