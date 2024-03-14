#pragma once

#include "ir/Ir.h"

class IrPrintVisitor : public ir::Visitor {
  public:
    IrPrintVisitor(std::ostream& out) : m_out(out) {}

    void visit(ir::Function& function) override;
    void visit(ir::BasicBlock& block) override;

  private:
    std::ostream& m_out;
};
