#pragma once
#include "ir/Ir.h"
#include "ir/Visitor.h"

class ConstantFoldingVisitor : public ir::Visitor {
  public:
    using ir::Visitor::visit;

    void visit(ir::BasicBlock& block) override;
    void visit(ir::BinaryOp& binaryOp) override;
    void visit(ir::UnaryOp& unaryOp) override;
    void visit(ir::ConditionalJump& jump) override;

    bool changed() const { return m_changed; }

  private:
    std::unique_ptr<ir::Instruction>* m_currentInstruction;
    std::unique_ptr<ir::Terminator>* m_currentTerminator;
    bool m_changed = false;
};
