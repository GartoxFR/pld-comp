#pragma once

#include "ir/Ir.h"

class X86GenVisitor : public ir::Visitor {
  public:
    void visit(ir::BinaryOp& binaryOp) override;
    void visit(ir::Assignment& assignment) override;
    void visit(ir::BasicJump& jump) override;
};
