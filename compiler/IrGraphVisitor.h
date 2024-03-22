#pragma once

#include "ir/Ir.h"

//Visit IR in order to create the control flow graph
class IrGraphVisitor : public ir::Visitor {
  public:
    IrGraphVisitor(std::ostream& out) : m_out(out) {}

    void visit(ir::Function& function) override;
    void visit(ir::BasicBlock& block) override;
    void visit(ir::BasicJump& jump) override;
    void visit(ir::ConditionalJump& jump) override;

  private:
    std::ostream& m_out;
    std::string m_currentBlockLabel;
};
