#include "ConstantFolding.h"
#include "ir/Instructions.h"
#include <variant>

using namespace ir;
void ConstantFoldingVisitor::visit(ir::BasicBlock& block) {

    for (auto& instr : block.instructions()) {
        m_currentInstruction = &instr;
        visitBase(*instr);
    }

    if (block.terminator()) {
        m_currentTerminator = &block.terminator();
        visitBase(*block.terminator());
    }
}

void ConstantFoldingVisitor::visit(ir::BinaryOp& binaryOp) {
    if (!std::holds_alternative<Immediate>(binaryOp.left()) || !std::holds_alternative<Immediate>(binaryOp.right())) {
        return;
    }

    Immediate left = std::get<Immediate>(binaryOp.left());
    Immediate right = std::get<Immediate>(binaryOp.right());

    Immediate folded{0};
    switch (binaryOp.operation()) {
        case BinaryOpKind::ADD: folded = Immediate(left.value() + right.value()); break;
        case BinaryOpKind::SUB: folded = Immediate(left.value() - right.value()); break;
        case BinaryOpKind::MUL: folded = Immediate(left.value() * right.value()); break;
        case BinaryOpKind::DIV: folded = Immediate(left.value() / right.value()); break;
        case BinaryOpKind::MOD: folded = Immediate(left.value() % right.value()); break;
        case BinaryOpKind::EQ: folded = Immediate(left.value() == right.value()); break;
        case BinaryOpKind::NEQ: folded = Immediate(left.value() != right.value()); break;
        case BinaryOpKind::CMP_L: folded = Immediate(left.value() < right.value()); break;
        case BinaryOpKind::CMP_G: folded = Immediate(left.value() > right.value()); break;
        case BinaryOpKind::CMP_LE: folded = Immediate(left.value() <= right.value()); break;
        case BinaryOpKind::CMP_GE: folded = Immediate(left.value() >= right.value()); break;
        case BinaryOpKind::BIT_AND: folded = Immediate(left.value() & right.value()); break;
        case BinaryOpKind::BIT_XOR: folded = Immediate(left.value() ^ right.value()); break;
        case BinaryOpKind::BIT_OR: folded = Immediate(left.value() | right.value()); break;
    }

    *m_currentInstruction = std::make_unique<Assignment>(binaryOp.destination(), folded);
    m_changed = true;
}

void ConstantFoldingVisitor::visit(ir::UnaryOp& unaryOp) {
    if (!std::holds_alternative<Immediate>(unaryOp.operand())) {
        return;
    }

    Immediate operand = std::get<Immediate>(unaryOp.operand());
    Immediate folded{0};
    switch (unaryOp.operation()) {
        case UnaryOpKind::MINUS: folded = Immediate(-operand.value()); break;
        case UnaryOpKind::NOT: folded = Immediate(!operand.value()); break;
    }
    *m_currentInstruction = std::make_unique<Assignment>(unaryOp.destination(), folded);
    m_changed = true;
}

void ConstantFoldingVisitor::visit(ir::ConditionalJump& jump) {
    if (!std::holds_alternative<Immediate>(jump.condition())) {
        return;
    }

    BasicBlock* target = std::get<Immediate>(jump.condition()).value() ? jump.trueTarget() : jump.falseTarget();

    *m_currentTerminator = std::make_unique<BasicJump>(target);
    m_changed = true;
}
