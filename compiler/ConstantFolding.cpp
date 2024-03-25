#include "ConstantFolding.h"
#include "ir/Instructions.h"
#include <stdexcept>
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
    if (tryFoldSpecialBinaryOp(binaryOp)) {
        return;
    }

    if (!std::holds_alternative<Immediate>(binaryOp.left()) || !std::holds_alternative<Immediate>(binaryOp.right())) {
        return;
    }

    Immediate left = std::get<Immediate>(binaryOp.left());
    Immediate right = std::get<Immediate>(binaryOp.right());
    auto type = binaryOp.destination().type();

    auto computeImmediate = [&binaryOp, type](auto left, auto right) {
        switch (binaryOp.operation()) {
            case BinaryOpKind::ADD: return Immediate(left + right, type);
            case BinaryOpKind::SUB: return Immediate(left - right, type);
            case BinaryOpKind::MUL: return Immediate(left * right, type);
            case BinaryOpKind::DIV: return Immediate(left / right, type);
            case BinaryOpKind::MOD: return Immediate(left % right, type);
            case BinaryOpKind::EQ: return Immediate(left == right, type);
            case BinaryOpKind::NEQ: return Immediate(left != right, type);
            case BinaryOpKind::CMP_L: return Immediate(left < right, type);
            case BinaryOpKind::CMP_G: return Immediate(left > right, type);
            case BinaryOpKind::CMP_LE: return Immediate(left <= right, type);
            case BinaryOpKind::CMP_GE: return Immediate(left >= right, type);
            case BinaryOpKind::BIT_AND: return Immediate(left & right, type);
            case BinaryOpKind::BIT_XOR: return Immediate(left ^ right, type);
            case BinaryOpKind::BIT_OR: return Immediate(left | right, type);
            default: throw std::runtime_error("Unhandled binary op");
        }
    };

    Immediate folded = [&]() {
        switch (type->size()) {
            case 8: return computeImmediate(left.value64(), right.value64());
            case 4: return computeImmediate(left.value32(), right.value32());
            case 2: return computeImmediate(left.value16(), right.value16());
            case 1: return computeImmediate(left.value8(), right.value8());
            default: throw std::runtime_error("Unsupported type size");
        }
    }();

    *m_currentInstruction = std::make_unique<Assignment>(binaryOp.destination(), folded);
    m_changed = true;
}

bool ConstantFoldingVisitor::tryFoldSpecialBinaryOp(ir::BinaryOp& binaryOp) {
    auto isImmediateEqual = [](RValue rvalue, int64_t value) -> bool {
        if (std::holds_alternative<Immediate>(rvalue)) {
            Immediate imm = std::get<Immediate>(rvalue);
            switch (imm.type()->size()) {
                case 8: return imm.value64() == value;
                case 4: return imm.value32() == value;
                case 2: return imm.value16() == value;
                case 1: return imm.value8() == value;
                default: return false;
            }
        }

        return false;
    };

    switch (binaryOp.operation()) {
        case BinaryOpKind::ADD:
            if (isImmediateEqual(binaryOp.right(), 0)) {
                *m_currentInstruction = std::make_unique<Assignment>(binaryOp.destination(), binaryOp.left());
                return true;
            }
            if (isImmediateEqual(binaryOp.left(), 0)) {
                *m_currentInstruction = std::make_unique<Assignment>(binaryOp.destination(), binaryOp.right());
                return true;
            }
            break;
        case BinaryOpKind::SUB:
            if (isImmediateEqual(binaryOp.right(), 0)) {
                *m_currentInstruction = std::make_unique<Assignment>(binaryOp.destination(), binaryOp.left());
                return true;
            }
            if (isImmediateEqual(binaryOp.left(), 0)) {
                *m_currentInstruction = std::make_unique<UnaryOp>(binaryOp.destination(), binaryOp.right(), UnaryOpKind::MINUS);
                return true;
            }
            break;
        case BinaryOpKind::MUL:
            if (isImmediateEqual(binaryOp.right(), 0)) {
                *m_currentInstruction = std::make_unique<Assignment>(binaryOp.destination(), binaryOp.right());
                return true;
            }
            if (isImmediateEqual(binaryOp.left(), 0)) {
                *m_currentInstruction = std::make_unique<Assignment>(binaryOp.destination(), binaryOp.left());
                return true;
            }
            if (isImmediateEqual(binaryOp.right(), 1)) {
                *m_currentInstruction = std::make_unique<Assignment>(binaryOp.destination(), binaryOp.left());
                return true;
            }
            if (isImmediateEqual(binaryOp.left(), 1)) {
                *m_currentInstruction = std::make_unique<Assignment>(binaryOp.destination(), binaryOp.right());
                return true;
            }
            break;

        default: break;
    }

    return false;
}

void ConstantFoldingVisitor::visit(ir::UnaryOp& unaryOp) {
    if (!std::holds_alternative<Immediate>(unaryOp.operand())) {
        return;
    }

    Immediate operand = std::get<Immediate>(unaryOp.operand());
    auto type = operand.type();

    auto computeImmediate = [&unaryOp, type](auto operand) {
        switch (unaryOp.operation()) {
            case UnaryOpKind::MINUS: return Immediate(-operand, type);
            case UnaryOpKind::NOT: return Immediate(!operand, type);
            default: throw std::runtime_error("Unhandled unary op");
        }
    };

    Immediate folded = [&]() {
        switch (type->size()) {
            case 8: return computeImmediate(operand.value64());
            case 4: return computeImmediate(operand.value32());
            case 2: return computeImmediate(operand.value16());
            case 1: return computeImmediate(operand.value8());
            default: throw std::runtime_error("Unsupported type size");
        }
    }();

    *m_currentInstruction = std::make_unique<Assignment>(unaryOp.destination(), folded);
    m_changed = true;
}

void ConstantFoldingVisitor::visit(ir::Cast& cast) {
    auto sourceType = std::visit([](auto val) { return val.type(); }, cast.source());
    auto destType = cast.destination().type();

    // Source and dest types have the same bit representation so we can just assign
    if (sourceType->size() == destType->size()) {
        *m_currentInstruction = std::make_unique<Assignment>(cast.destination(), cast.source());
        return;
    }

    if (!std::holds_alternative<Immediate>(cast.source())) {
        return;
    }

    Immediate source = std::get<Immediate>(cast.source());
    auto type = source.type();

    Immediate folded = [&]() {
        switch (type->size()) {
            case 8: return Immediate(source.value64(), destType);
            case 4: return Immediate(source.value32(), destType);
            case 2: return Immediate(source.value16(), destType);
            case 1: return Immediate(source.value8(), destType);
            default: throw std::runtime_error("Unsupported type size");
        }
    }();

    *m_currentInstruction = std::make_unique<Assignment>(cast.destination(), folded);
    m_changed = true;
}

void ConstantFoldingVisitor::visit(ir::ConditionalJump& jump) {
    if (!std::holds_alternative<Immediate>(jump.condition())) {
        return;
    }

    Immediate condition = std::get<Immediate>(jump.condition());
    bool b = false;
    switch (condition.type()->size()) {
        case 8: b = condition.value64(); break;
        case 4: b = condition.value32(); break;
        case 2: b = condition.value16(); break;
        case 1: b = condition.value8(); break;
        default: throw std::runtime_error("Unsupported type size");
    }
    BasicBlock* target = b ? jump.trueTarget() : jump.falseTarget();

    *m_currentTerminator = std::make_unique<BasicJump>(target);
    m_changed = true;
}
