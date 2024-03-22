#include "./X86GenVisitor.h"
#include "ir/Instructions.h"
#include <stdexcept>
#include <string_view>
#include <variant>

using namespace ir;

static constexpr Register CALL_REGISTER[] = {
    Register::RDI,
    Register::RSI,
    Register::RDX,
    Register::RCX,
};

void X86GenVisitor::visit(ir::Function& function) {
    m_out << ".global " << function.name() << "\n";
    m_out << function.name() << ":\n";
    m_out << "    pushq   %rbp\n";
    m_out << "    movq    %rsp, %rbp\n";
    m_out << "    subq    $" << function.locals().size() * 4 << ", %rsp\n";

    for (size_t i = 0; i < function.argCount(); i++) {
        auto type = function.locals()[i + 1].type();
        auto suffix = getSuffix(type->size());
        SizedRegister reg = {CALL_REGISTER[i], type->size()};
        Local local(i + 1, type);
        emit("mov", suffix, reg, local);
    }

    visit(*function.prologue());
    for (auto& block : function.blocks()) {
        visit(*block);
    }
    visit(*function.epilogue());
    loadEax(function.returnLocal());

    m_out << "    movq    %rbp, %rsp\n";
    m_out << "    popq    %rbp\n";
    m_out << "    ret\n";
}

void X86GenVisitor::visit(ir::BasicBlock& block) {
    m_out << block.label() << ":\n";
    for (auto& instr : block.instructions()) {
        visitBase(*instr);
    }

    if (block.terminator()) {
        visitBase(*block.terminator());
    }
}

void X86GenVisitor::emitSimpleArithmetic(std::string_view instruction, const BinaryOp& binaryOp) {
    auto size = binaryOp.destination().type()->size();
    SizedRegister rax = {Register::RAX, size};
    auto suffix = getSuffix(size);
    emit("mov", suffix, binaryOp.left(), rax);
    emit(instruction, suffix, binaryOp.right(), rax);
    emit("mov", suffix, rax, binaryOp.destination());
}

void X86GenVisitor::emitCmp(std::string_view instruction, const ir::BinaryOp& binaryOp) {
    auto type = binaryOp.destination().type();
    auto size = type->size();
    SizedRegister rax = {Register::RAX, size};
    SizedRegister rdx = {Register::RDX, size};
    auto suffix = getSuffix(size);
    emit("mov", suffix, binaryOp.left(), rax);
    emit("mov", suffix, Immediate(1, type), rdx);
    emit("cmp", suffix, binaryOp.right(), rax);
    emit("mov", suffix, Immediate(0, type), rax);
    emit(instruction, suffix, rdx, rax);
    emit("mov", suffix, rax, binaryOp.destination());
}
void X86GenVisitor::emitDiv(bool modulo, const ir::BinaryOp& binaryOp) {
    auto type = binaryOp.destination().type();
    auto size = type->size();
    SizedRegister rax = {Register::RAX, size};
    SizedRegister rdx = {Register::RDX, size};
    SizedRegister rbx = {Register::RBX, size};
    auto suffix = getSuffix(size);
    emit("mov", suffix, binaryOp.left(), rax);

    switch (size) {
        case 8: emit("cqto"); break;
        case 4: emit("cltd"); break;
        case 2: emit("cwtd"); break;
        case 1: emit("cbtw"); break;
        default: throw std::runtime_error("Unsupported size");
    }

    if (std::holds_alternative<Local>(binaryOp.right())) {
        Local rightLocal = std::get<Local>(binaryOp.right());
        emit("idiv", suffix, rightLocal);
    } else {
        Immediate rightImmediate = std::get<Immediate>(binaryOp.right());
        emit("mov", suffix, rightImmediate, rbx);
        emit("idiv", suffix, rbx);
    }

    if (modulo) {
        emit("mov", suffix, rdx, binaryOp.destination());
    } else {
        emit("mov", suffix, rax, binaryOp.destination());
    }
}

void X86GenVisitor::visit(ir::BinaryOp& binaryOp) {
    switch (binaryOp.operation()) {
        case BinaryOpKind::ADD: emitSimpleArithmetic("add", binaryOp); break;
        case BinaryOpKind::SUB: emitSimpleArithmetic("sub", binaryOp); break;
        case BinaryOpKind::MUL: emitSimpleArithmetic("imul", binaryOp); break;
        case BinaryOpKind::DIV: emitDiv(false, binaryOp); break;
        case BinaryOpKind::MOD: emitDiv(true, binaryOp); break;
        case BinaryOpKind::EQ: emitCmp("cmove", binaryOp); break;
        case BinaryOpKind::NEQ: emitCmp("cmovne", binaryOp); break;
        case BinaryOpKind::CMP_L: emitCmp("cmovl", binaryOp); break;
        case BinaryOpKind::CMP_G: emitCmp("cmovg", binaryOp); break;
        case BinaryOpKind::CMP_LE: emitCmp("cmovle", binaryOp); break;
        case BinaryOpKind::CMP_GE: emitCmp("cmovge", binaryOp); break;
        case BinaryOpKind::BIT_AND: emitSimpleArithmetic("and ", binaryOp); break;
        case BinaryOpKind::BIT_XOR: emitSimpleArithmetic("xor ", binaryOp); break;
        case BinaryOpKind::BIT_OR: emitSimpleArithmetic("or ", binaryOp); break;
    }
}

void X86GenVisitor::visit(ir::UnaryOp& unaryOp) {
    auto type = unaryOp.destination().type();
    auto size = type->size();
    SizedRegister rax = {Register::RAX, size};
    SizedRegister rdx = {Register::RDX, size};
    auto suffix = getSuffix(size);
    emit("mov", suffix, unaryOp.operand(), rax);
    switch (unaryOp.operation()) {
        case UnaryOpKind::MINUS: emit("neg", suffix, rax);

        case UnaryOpKind::NOT:
            emit("mov", suffix, Immediate(1, type), rdx);
            emit("test", rax, rax);
            emit("mov", suffix, Immediate(0, type), rax);
            emit("cmovz", suffix, rdx, rax);
            break;
    }
    emit("mov", suffix, rax, unaryOp.destination());
}

void X86GenVisitor::visit(ir::Assignment& assignment) {
    SizedRegister rax = {Register::RAX, assignment.destination().type()->size()};
    auto suffix = getSuffix(assignment.destination().type()->size());

    if (std::holds_alternative<Local>(assignment.source())) {
        Local source = std::get<Local>(assignment.source());
        emit("mov", suffix, source, rax);
        emit("mov", suffix, rax, assignment.destination());
    } else {
        Immediate source = std::get<Immediate>(assignment.source());
        emit("mov", suffix, source, assignment.destination());
    }
}

void X86GenVisitor::visit(ir::BasicJump& jump) { m_out << "    jmp     " << jump.target()->label() << "\n"; }
void X86GenVisitor::visit(ir::ConditionalJump& jump) {
    auto size = std::visit([](auto val) { return val.type()->size(); }, jump.condition());
    auto suffix = getSuffix(size);
    SizedRegister rax = {Register::RAX, size};
    emit("mov", suffix, jump.condition(), rax);
    emit("test", rax, rax);
    m_out << "    jne     " << jump.trueTarget()->label() << "\n";
    m_out << "    jmp     " << jump.falseTarget()->label() << "\n";
}

void X86GenVisitor::visit(ir::Call& call) {
    for (size_t i = 0; i < call.args().size(); i++) {
        RValue arg = call.args()[i];
        auto type = std::visit([](auto val) { return val.type(); }, arg);
        auto suffix = getSuffix(type->size());
        SizedRegister reg = {CALL_REGISTER[i], type->size()};
        emit("mov", suffix, arg, reg);
    }

    m_out << "    call    " << call.name() << "\n";
    saveEax(call.destination());
}

void X86GenVisitor::visit(ir::Cast& cast) {
    auto sourceType = std::visit([](auto val) { return val.type(); }, cast.source());
    auto destinationType = cast.destination().type();
    SizedRegister rax = {Register::RAX, destinationType->size()};
    if (sourceType->size() < destinationType->size()) {
        std::stringstream suffix;
        suffix << getSuffix(sourceType->size()) << getSuffix(destinationType->size());
        emit("movs", std::string_view(suffix.str()), cast.source(), rax);
        emit("mov", getSuffix(destinationType->size()), rax, cast.destination());
    } else {
        auto suffix = getSuffix(destinationType->size());
        emit("mov", suffix, cast.source(), rax);
        emit("mov", suffix, rax, cast.destination());
    }
}
