#pragma once

#include "ir/Instructions.h"
#include "ir/Ir.h"
#include <ostream>

class X86GenVisitor : public ir::Visitor {
  public:
    X86GenVisitor(std::ostream& out) : m_out(out) {}

    void visit(ir::Function& function) override;
    void visit(ir::BasicBlock& block) override;
    void visit(ir::BinaryOp& binaryOp) override;
    void visit(ir::UnaryOp& unaryOp) override;
    void visit(ir::Assignment& assignment) override;
    void visit(ir::BasicJump& jump) override;
    void visit(ir::ConditionalJump& jump) override;
    void visit(ir::Call& call) override;

    void emitSimpleArithmetic(std::string_view instr, const ir::BinaryOp& binaryOp);
    void emitEq(const ir::BinaryOp& binaryOp);
    void emitNeq(const ir::BinaryOp& binaryOp);
    void emitCmp(std::string_view instruction, const ir::BinaryOp& binaryOp);
    void emitDiv(bool modulo, const ir::BinaryOp& binaryOp);

  private:
    std::ostream& m_out;

    void loadEax(const ir::Local& local) { m_out << "    movl    " << variableLocation(local) << ", %eax\n"; }
    void loadEax(const ir::Immediate& immediate) { m_out << "    movl    $" << immediate.value() << ", %eax\n"; }

    void loadEax(const ir::RValue& rvalue) {
        if (std::holds_alternative<ir::Local>(rvalue)) {
            loadEax(std::get<ir::Local>(rvalue));
        } else {
            loadEax(std::get<ir::Immediate>(rvalue));
        }
    }

    void saveEax(const ir::Local& local) { m_out << "    movl    %eax, " << variableLocation(local) << "\n"; }

    std::string variableLocation(const ir::Local& local) {
        std::stringstream ss;
        ss << "-" << (local.id() + 1) * 4 << "(%rbp)";
        return ss.str();
    }
};
