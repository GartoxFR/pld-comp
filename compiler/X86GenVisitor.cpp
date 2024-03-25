#include "./X86GenVisitor.h"
#include "BlockDependance.h"
#include "BlockLivenessAnalysis.h"
#include "InterferenceGraph.h"
#include "PointedLocalGatherer.h"
#include "RegisterAllocation.h"
#include "Type.h"
#include "ir/Instructions.h"
#include <fstream>
#include <set>
#include <stdexcept>
#include <string_view>
#include <variant>
#include <ranges>

using namespace ir;

static constexpr Register CALL_REGISTER[] = {
    Register::RDI, Register::RSI, Register::RDX, Register::RCX, Register::R8, Register::R9,
};

void X86GenVisitor::visit(ir::Function& function) {
    m_currentFunction = &function;
    m_localsInRegister.clear();
    m_localsOnStack.clear();

    PointedLocals pointedLocals = computePointedLocals(function);
    DependanceMap dependanceMap = computeDependanceMap(function);
    InterferenceGraph interferenceGraph{function.locals().size()};
    // Compute the interferenceGraph using the block liveness analysis
    computeBlockLivenessAnalysis(function, dependanceMap, &interferenceGraph);

    std::ofstream debugFile(function.name() + ".ig.dot");
    interferenceGraph.printDot(debugFile);

    std::cerr << function.name() << ": " << std::endl;

    std::vector<Register> usableRegisters{Register::RBX, Register::R12, Register::R13, Register::R14, Register::R15};

    RegisterAllocationResult registerAllocation =
        computeRegisterAllocation(function, pointedLocals, interferenceGraph, usableRegisters.size());

    std::set<Register> usedRegisters;
    for (const auto& [localId, registerId] : registerAllocation.registers()) {
        Local local{localId, function.locals()[localId].type()};
        Register reg = usableRegisters[registerId];
        m_localsInRegister.insert({local, reg});
        usedRegisters.insert(reg);
    }

    size_t stackPos = 8;
    for (const auto& localId : registerAllocation.spilled()) {
        Local local{localId, function.locals()[localId].type()};
        m_localsOnStack.insert({local, stackPos});
        stackPos += 8;
    }

    m_out << ".section .text\n";
    m_out << ".global " << function.name() << "\n";
    m_out << function.name() << ":\n";
    m_out << "    pushq   %rbp\n";
    for (auto reg : usedRegisters) {
        emit("pushq", SizedRegister{reg, 8});
    }
    m_out << "    movq    %rsp, %rbp\n";
    m_out << "    subq    $" << m_localsOnStack.size() * 8 << ", %rsp\n";

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
    SizedRegister rax{Register::RAX, function.returnLocal().type()->size()};
    auto suffix = getSuffix(function.returnLocal().type()->size());
    emit("mov", suffix, function.returnLocal(), rax);

    m_out << "    movq    %rbp, %rsp\n";

    for (auto it = usedRegisters.rbegin(); it != usedRegisters.rend(); ++it) {
        emit("popq", SizedRegister{*it, 8});
    }
    m_out << "    popq    %rbp\n";
    m_out << "    ret\n";

    m_out << "\n";
    m_out << ".section .rodata\n";
    int i = 0;
    for (const auto& literal : function.literals()) {
        m_out << literalLabel(StringLiteral(i)) << ":\n";
        m_out << "    .asciz\t\"" << literal << "\"\n";
        i++;
    }
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
    SizedRegister reg = {Register::RAX, size};
    auto suffix = getSuffix(size);

    auto rightReg = rvalueRegister(binaryOp.right());
    auto destReg = variableRegister(binaryOp.destination());

    if (destReg && (!rightReg || rightReg.value() != destReg.value())) {
        reg.reg = destReg.value();
        emit("mov", suffix, binaryOp.left(), reg);
        emit(instruction, suffix, binaryOp.right(), reg);
    } else {
        emit("mov", suffix, binaryOp.left(), reg);
        emit(instruction, suffix, binaryOp.right(), reg);
        emit("mov", suffix, reg, binaryOp.destination());
    }
}

void X86GenVisitor::emitSimpleArithmeticCommutative(std::string_view instruction, const ir::BinaryOp& binaryOp) {

    auto size = binaryOp.destination().type()->size();
    SizedRegister reg = {Register::RAX, size};
    auto suffix = getSuffix(size);

    auto leftReg = rvalueRegister(binaryOp.left());
    auto rightReg = rvalueRegister(binaryOp.right());
    auto destReg = variableRegister(binaryOp.destination());

    if (destReg && (!rightReg || rightReg.value() != destReg.value())) {
        reg.reg = destReg.value();
        emit("mov", suffix, binaryOp.left(), reg);
        emit(instruction, suffix, binaryOp.right(), reg);
    } else if (destReg && (!leftReg || leftReg.value() != destReg.value())) {
        reg.reg = destReg.value();
        emit("mov", suffix, binaryOp.right(), reg);
        emit(instruction, suffix, binaryOp.left(), reg);
    } else {
        emit("mov", suffix, binaryOp.left(), reg);
        emit(instruction, suffix, binaryOp.right(), reg);
        emit("mov", suffix, reg, binaryOp.destination());
    }
}

void X86GenVisitor::emitCmp(std::string_view instruction, const ir::BinaryOp& binaryOp) {
    auto operandSize = std::visit([](auto val) { return val.type()->size(); }, binaryOp.left());
    SizedRegister rax = {Register::RAX, operandSize};
    auto suffix = getSuffix(operandSize);

    SizedRegister sourceReg = rax;
    if (auto regOpt = rvalueRegister(binaryOp.left()); regOpt) {
        sourceReg.reg = regOpt.value();
    } else if (auto regOpt = rvalueRegister(binaryOp.right()); regOpt) {
        sourceReg.reg = regOpt.value();
    } else {
        emit("mov", suffix, binaryOp.left(), sourceReg);
    }

    emit("cmp", suffix, binaryOp.right(), sourceReg);

    auto optionalReg = variableRegister(binaryOp.destination());
    if (optionalReg) {
        SizedRegister reg{optionalReg.value(), 1};
        emit(instruction, reg);
    } else {
        SizedRegister al{Register::RAX, 1};
        emit(instruction, al);
        emit("mov", getSuffix(1), al, binaryOp.destination());
    }
}

void X86GenVisitor::emitDiv(bool modulo, const ir::BinaryOp& binaryOp) {
    auto type = binaryOp.destination().type();
    auto size = type->size();
    SizedRegister rax = {Register::RAX, size};
    SizedRegister rdx = {Register::RDX, size};
    SizedRegister rcx = {Register::RCX, size};
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
        emit("mov", suffix, rightImmediate, rcx);
        emit("idiv", suffix, rcx);
    }

    if (modulo) {
        emit("mov", suffix, rdx, binaryOp.destination());
    } else {
        emit("mov", suffix, rax, binaryOp.destination());
    }
}

void X86GenVisitor::visit(ir::BinaryOp& binaryOp) {
    switch (binaryOp.operation()) {
        case BinaryOpKind::ADD: emitSimpleArithmeticCommutative("add", binaryOp); break;
        case BinaryOpKind::SUB: emitSimpleArithmetic("sub", binaryOp); break;
        case BinaryOpKind::MUL: emitSimpleArithmeticCommutative("imul", binaryOp); break;
        case BinaryOpKind::DIV: emitDiv(false, binaryOp); break;
        case BinaryOpKind::MOD: emitDiv(true, binaryOp); break;
        case BinaryOpKind::EQ: emitCmp("sete", binaryOp); break;
        case BinaryOpKind::NEQ: emitCmp("setne", binaryOp); break;
        case BinaryOpKind::CMP_L: emitCmp("setl", binaryOp); break;
        case BinaryOpKind::CMP_G: emitCmp("setg", binaryOp); break;
        case BinaryOpKind::CMP_LE: emitCmp("setle", binaryOp); break;
        case BinaryOpKind::CMP_GE: emitCmp("setge", binaryOp); break;
        case BinaryOpKind::BIT_AND: emitSimpleArithmeticCommutative("and", binaryOp); break;
        case BinaryOpKind::BIT_XOR: emitSimpleArithmeticCommutative("xor", binaryOp); break;
        case BinaryOpKind::BIT_OR: emitSimpleArithmeticCommutative("or", binaryOp); break;
    }
}

void X86GenVisitor::visit(ir::UnaryOp& unaryOp) {
    auto size = std::visit([](auto val) { return val.type()->size(); }, unaryOp.operand());
    SizedRegister rax = {Register::RAX, size};
    SizedRegister al = {Register::RAX, types::BOOL->size()};

    auto suffix = getSuffix(size);

    switch (unaryOp.operation()) {
        case UnaryOpKind::MINUS: {
            auto optionalReg = variableRegister(unaryOp.destination());
            if (optionalReg) {
                SizedRegister reg = SizedRegister{optionalReg.value(), unaryOp.destination().type()->size()};
                emit("mov", suffix, unaryOp.operand(), reg);
                emit("neg", suffix, reg);
            } else {
                emit("mov", suffix, unaryOp.operand(), rax);
                emit("neg", suffix, rax);
                emit("mov", suffix, rax, unaryOp.destination());
            }
        } break;
        case UnaryOpKind::NOT: {
            auto optionalReg = rvalueRegister(unaryOp.operand());
            SizedRegister sourceReg = rax;
            if (optionalReg) {
                sourceReg.reg = optionalReg.value();
            } else {
                emit("mov", suffix, unaryOp.operand(), rax);
            }

            emit("test", sourceReg, sourceReg);

            optionalReg = variableRegister(unaryOp.destination());
            if (optionalReg) {
                SizedRegister reg{optionalReg.value(), unaryOp.destination().type()->size()};
                emit("setz", reg);
            } else {
                emit("setz", al);
                emit("movb", al, unaryOp.destination());
            }
        } break;
    }
}

void X86GenVisitor::visit(ir::Assignment& assignment) {
    auto suffix = getSuffix(assignment.destination().type()->size());

    if (std::holds_alternative<Local>(assignment.source())) {
        Local source = std::get<Local>(assignment.source());
        Register destReg = Register::RAX;

        auto destOpt = variableRegister(assignment.destination());
        if (destOpt) {
            destReg = destOpt.value();
        }

        SizedRegister dest{destReg, source.type()->size()};
        emit("mov", suffix, source, dest);
        if (!destOpt) {
            emit("mov", suffix, dest, assignment.destination());
        }
    } else {
        Immediate source = std::get<Immediate>(assignment.source());
        emit("mov", suffix, source, assignment.destination());
    }
}

void X86GenVisitor::visit(ir::BasicJump& jump) { m_out << "    jmp     " << jump.target()->label() << "\n"; }
void X86GenVisitor::visit(ir::ConditionalJump& jump) {
    auto size = std::visit([](auto val) { return val.type()->size(); }, jump.condition());
    auto suffix = getSuffix(size);
    SizedRegister reg = {Register::RAX, size};
    auto optionalReg = rvalueRegister(jump.condition());
    if (optionalReg) {
        reg.reg = optionalReg.value();
    } else {
        emit("mov", suffix, jump.condition(), reg);
    }
    emit("test", reg, reg);
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

    if (call.variadic()) {
        emit("movq", Immediate(0, types::LONG), SizedRegister(Register::RAX, 8));
    }

    emit("call", Label(call.name()));
    if (call.destination().type()->size() > 0) {
        SizedRegister rax = {Register::RAX, call.destination().type()->size()};
        auto suffix = getSuffix(rax.size);
        emit("mov", suffix, rax, call.destination());
    }
}

void X86GenVisitor::visit(ir::Cast& cast) {
    auto sourceType = std::visit([](auto val) { return val.type(); }, cast.source());
    auto destinationType = cast.destination().type();
    SizedRegister reg = {Register::RAX, destinationType->size()};
    if (sourceType->size() < destinationType->size()) {
        std::stringstream suffix;
        suffix << getSuffix(sourceType->size()) << getSuffix(destinationType->size());
        auto optionalReg = variableRegister(cast.destination());
        if (optionalReg) {
            reg.reg = optionalReg.value();
            emit("movs", std::string_view(suffix.str()), cast.source(), reg);
        } else {
            emit("movs", std::string_view(suffix.str()), cast.source(), reg);
            emit("mov", getSuffix(destinationType->size()), reg, cast.destination());
        }
    } else {
        auto suffix = getSuffix(destinationType->size());
        auto optionalReg = variableRegister(cast.destination());
        if (optionalReg) {
            reg.reg = optionalReg.value();
            emit("mov", suffix, cast.source(), reg);
        } else {
            emit("mov", suffix, cast.source(), reg);
            emit("mov", suffix, reg, cast.destination());
        }
    }
}

void X86GenVisitor::visit(ir::PointerRead& pointerRead) {
    auto type = pointerRead.destination().type();
    SizedRegister destReg{Register::RAX, type->size()};
    SizedRegister sourceReg{Register::RDX, 8};
    auto suffix = getSuffix(destReg.size);

    auto optionalReg = rvalueRegister(pointerRead.address());
    if (optionalReg) {
        sourceReg.reg = optionalReg.value();
    } else {
        emit("movq", pointerRead.address(), sourceReg);
    }

    optionalReg = variableRegister(pointerRead.destination());
    if (optionalReg) {
        destReg.reg = optionalReg.value();
        emit("mov", suffix, Deref(sourceReg), destReg);
    } else {

        emit("mov", suffix, Deref(sourceReg), destReg);
        emit("mov", suffix, destReg, pointerRead.destination());
    }
}

void X86GenVisitor::visit(ir::PointerWrite& pointerWrite) {
    auto type = std::visit([](auto val) { return val.type(); }, pointerWrite.source());
    SizedRegister sourceReg{Register::RAX, type->size()};
    SizedRegister addressReg{Register::RDX, 8};
    auto suffix = getSuffix(sourceReg.size);

    auto optionalReg = rvalueRegister(pointerWrite.address());
    if (optionalReg) {
        addressReg.reg = optionalReg.value();
    } else {
        emit("movq", pointerWrite.address(), addressReg);
    }

    optionalReg = rvalueRegister(pointerWrite.source());
    if (optionalReg) {
        sourceReg.reg = optionalReg.value();
        emit("mov", suffix, sourceReg, Deref(addressReg));
    } else if (std::holds_alternative<Immediate>(pointerWrite.source())) {
        emit("mov", suffix, std::get<Immediate>(pointerWrite.source()), Deref(addressReg));
    } else {
        emit("mov", suffix, pointerWrite.source(), sourceReg);
        emit("mov", suffix, sourceReg, Deref(addressReg));
    }
}

void X86GenVisitor::visit(ir::AddressOf& addressOf) {
    SizedRegister destReg = {Register::RAX, 8};
    if (std::holds_alternative<Local>(addressOf.source())) {
        Local source = std::get<Local>(addressOf.source());
        auto optionalReg = variableRegister(addressOf.destination());
        if (optionalReg) {
            destReg.reg = optionalReg.value();
            emit("leaq", source, destReg);
        } else {
            emit("leaq", source, destReg);
            emit("movq", destReg, addressOf.destination());
        }
    } else {
        StringLiteral literal = std::get<StringLiteral>(addressOf.source());
        auto optionalReg = variableRegister(addressOf.destination());
        if (optionalReg) {
            destReg.reg = optionalReg.value();
            emit("leaq", Label(literalLabel(literal)), destReg);
        } else {
            emit("leaq", Label(literalLabel(literal)), destReg);
            emit("movq", destReg, addressOf.destination());
        }
    }
}
