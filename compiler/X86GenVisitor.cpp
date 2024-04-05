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

constexpr static uint32_t ARGS_IN_REGISTER = 6;

static constexpr Register CALL_REGISTER[ARGS_IN_REGISTER] = {
    Register::RDI, Register::RSI, Register::RDX, Register::RCX, Register::R8, Register::R9,
};

std::set<Register> X86GenVisitor::registerAllocation(
    const ir::Function& function, const PointedLocals& pointedLocals, const InterferenceGraph& interferenceGraph
) {
    std::vector<Register> usableRegisters{Register::R10, Register::R11, Register::RBX, Register::R12,
                                          Register::R13, Register::R14, Register::R15};

    RegisterAllocationResult registerAllocation =
        computeRegisterAllocation(function, pointedLocals, interferenceGraph, usableRegisters.size());

    std::set<Register> usedRegisters;
    for (const auto& [localId, registerId] : registerAllocation.registers()) {
        Local local{localId, function.locals()[localId].type()};

        if (localId == 0 && interferenceGraph.neighbors(localId).empty()) {
            m_localsInRegister.insert({local, Register::RAX});
            continue;
        }

        if (localId > 0 && localId <= function.argCount() && localId <= ARGS_IN_REGISTER) {
            // We might me able to let this argument in his register
            Register possibleReg = CALL_REGISTER[localId - 1];
            bool possible = possibleReg != Register::RDX && possibleReg != Register::RCX;
            for (const auto& [key, pair] : m_localsUsedThroughCalls) {
                if (pair.first.contains(local)) {
                    possible = false;
                }
            }

            if (possible) {
                m_localsInRegister.insert({local, possibleReg});
                continue;
            }
        }

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
    return usedRegisters;
}

void X86GenVisitor::visit(ir::Function& function) {
    m_currentFunction = &function;
    m_localsInRegister.clear();
    m_localsOnStack.clear();
    m_instructions.clear();

    // When we are called, there is the return address pushed
    m_stackAlignment = 1;

    PointedLocals pointedLocals = computePointedLocals(function);
    DependanceMap dependanceMap = computeDependanceMap(function);
    InterferenceGraph interferenceGraph{function.locals().size()};
    m_localsUsedThroughCalls.clear();
    // Compute the interferenceGraph using the block liveness analysis
    m_livenessAnalysis =
        computeBlockLivenessAnalysis(function, dependanceMap, &interferenceGraph, &m_localsUsedThroughCalls);


    std::set<Register> usedRegisters;
    if(m_doRegisterAllocation) {
        usedRegisters = registerAllocation(function, pointedLocals, interferenceGraph);
    } else {
        size_t stackPos = 8;
        for (size_t i = 0; i < function.locals().size(); i++) {
            Local local(i, function.locals()[i].type());
            m_localsOnStack.insert({local, stackPos});
            stackPos += 8;
        }
    }

    std::ofstream debugFile(function.name() + ".ig.dot");
    interferenceGraph.printDot(debugFile);

    std::cerr << function.name() << ": " << std::endl;

    bool needStack = !m_localsOnStack.empty();

    m_out << ".section .text\n";
    m_out << ".global " << function.name() << "\n";
    m_out << function.name() << ":\n";

    if (needStack) {
        emit("pushq", SizedRegister(Register::RBP, 8));
        m_stackAlignment++;
    }

    for (auto reg : usedRegisters) {
        if (preserved(reg)) {
            emit("pushq", SizedRegister{reg, 8});
            m_stackAlignment++;
        }
    }

    if (needStack) {
        emit("movq", SizedRegister(Register::RSP, 8), SizedRegister(Register::RBP, 8));
        emit("subq", Immediate(m_localsOnStack.size() * 8, types::LONG), SizedRegister(Register::RSP, 8));
        m_stackAlignment += m_localsOnStack.size();
    }

    for (size_t i = 0; i < function.argCount() && i < ARGS_IN_REGISTER; i++) {
        auto type = function.locals()[i + 1].type();
        auto suffix = getSuffix(type->size());
        SizedRegister reg = {CALL_REGISTER[i], type->size()};
        Local local(i + 1, type);
        if (!m_livenessAnalysis[function.prologue()].first.contains(local)) {
            continue;
        }
        emit("mov", suffix, reg, local);
    }

    for (size_t i = ARGS_IN_REGISTER; i < function.argCount(); i++) {
        auto type = function.locals()[i + 1].type();
        auto suffix = getSuffix(type->size());
        Local local(i + 1, type);
        if (!m_livenessAnalysis[function.prologue()].first.contains(local)) {
            continue;
        }
        auto optionalReg = variableRegister(local);
        DerefOffset argLocation(SizedRegister{Register::RSP, 8}, (i - ARGS_IN_REGISTER + m_stackAlignment) * 8);
        if (optionalReg.has_value()) {
            emit("mov", suffix, argLocation, SizedRegister(optionalReg.value(), type->size()));
        } else {
            SizedRegister rax(Register::RAX, type->size());
            emit("mov", suffix, argLocation, rax);
            emit("mov", suffix, rax, local);
        }
    }

    bool adjustAlignment = m_stackAlignment % 2 == 1 && m_localsUsedThroughCalls.size() != 0;

    if (adjustAlignment) {
        emit("pushq", SizedRegister(Register::RCX, 8));
        m_stackAlignment++;
    }

    visit(*function.prologue());
    for (auto& block : function.blocks()) {
        visit(*block);
    }
    visit(*function.epilogue());

    if (adjustAlignment) {
        emit("popq", SizedRegister(Register::RCX, 8));
        m_stackAlignment--;
    }

    SizedRegister rax{Register::RAX, function.returnLocal().type()->size()};
    if (function.returnLocal().type()->size() > 0) {
        auto suffix = getSuffix(function.returnLocal().type()->size());
        emit("mov", suffix, function.returnLocal(), rax);
    }

    if (needStack) {
        emit("movq", SizedRegister(Register::RBP, 8), SizedRegister(Register::RSP, 8));
    }

    for (auto it = usedRegisters.rbegin(); it != usedRegisters.rend(); ++it) {
        if (preserved(*it)) {
            emit("popq", SizedRegister{*it, 8});
            m_stackAlignment--;
        }
    }
    if (needStack) {
        emit("popq", SizedRegister(Register::RBP, 8));
        m_stackAlignment--;
    }
    emit("ret");

    simplifyAsm();
    printAsm();

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
    m_currentBlock = &block;
    m_optimizedCond = std::nullopt;
    emit(std::string_view(block.label()), std::string_view(":"));
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

void X86GenVisitor::emitCmp(std::string_view instructionSuffix, const ir::BinaryOp& binaryOp) {
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

    if (m_currentBlock->instructions().back().get() == &binaryOp &&
        !m_livenessAnalysis[m_currentBlock].second.contains(binaryOp.destination())) {
        ConditionalJump* jump = dynamic_cast<ConditionalJump*>(m_currentBlock->terminator().get());
        if (jump) {
            if (std::holds_alternative<Local>(jump->condition()) &&
                std::get<Local>(jump->condition()) == binaryOp.destination()) {
                m_optimizedCond = instructionSuffix;
                return;
            }
        }
    }

    auto optionalReg = variableRegister(binaryOp.destination());
    if (optionalReg) {
        SizedRegister reg{optionalReg.value(), 1};
        emit("set", instructionSuffix, reg);
    } else {
        SizedRegister al{Register::RAX, 1};
        emit("set", instructionSuffix, al);
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
        case BinaryOpKind::EQ: emitCmp("e", binaryOp); break;
        case BinaryOpKind::NEQ: emitCmp("ne", binaryOp); break;
        case BinaryOpKind::CMP_L: emitCmp("l", binaryOp); break;
        case BinaryOpKind::CMP_G: emitCmp("g", binaryOp); break;
        case BinaryOpKind::CMP_LE: emitCmp("le", binaryOp); break;
        case BinaryOpKind::CMP_GE: emitCmp("ge", binaryOp); break;
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

            if (m_currentBlock->instructions().back().get() == &unaryOp &&
                !m_livenessAnalysis[m_currentBlock].second.contains(unaryOp.destination())) {
                ConditionalJump* jump = dynamic_cast<ConditionalJump*>(m_currentBlock->terminator().get());
                if (jump) {
                    if (std::holds_alternative<Local>(jump->condition()) &&
                        std::get<Local>(jump->condition()) == unaryOp.destination()) {
                        m_optimizedCond = "z";
                        return;
                    }
                }
            }

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

void X86GenVisitor::visit(ir::BasicJump& jump) { emit("jmp", Label(jump.target()->label())); }
void X86GenVisitor::visit(ir::ConditionalJump& jump) {
    auto size = std::visit([](auto val) { return val.type()->size(); }, jump.condition());
    auto suffix = getSuffix(size);
    SizedRegister reg = {Register::RAX, size};

    if (m_optimizedCond.has_value()) {
        emit("j", m_optimizedCond.value(), Label(jump.trueTarget()->label()));
    } else {
        auto optionalReg = rvalueRegister(jump.condition());
        if (optionalReg) {
            reg.reg = optionalReg.value();
        } else {
            emit("mov", suffix, jump.condition(), reg);
        }
        emit("test", reg, reg);
        emit("jne", Label(jump.trueTarget()->label()));
    }
    emit("jmp", Label(jump.falseTarget()->label()));
}

void X86GenVisitor::visit(ir::Call& call) {
    const auto& [before, after] = m_localsUsedThroughCalls.at(&call);
    std::vector<Register> savedRegister;
    for (const auto& local : before) {
        if (!after.contains(local))
            continue;
        auto optReg = variableRegister(local);
        if (optReg && !preserved(optReg.value())) {
            emit("pushq", SizedRegister(optReg.value(), 8));
            savedRegister.push_back(optReg.value());
            m_stackAlignment++;
        }
    }

    for (size_t i = 0; i < call.args().size() && i < ARGS_IN_REGISTER; i++) {
        RValue arg = call.args()[i];
        auto type = std::visit([](auto val) { return val.type(); }, arg);
        auto suffix = getSuffix(type->size());
        SizedRegister reg = {CALL_REGISTER[i], type->size()};
        emit("mov", suffix, arg, reg);
    }

    if (call.args().size() > ARGS_IN_REGISTER) {
        m_stackAlignment += call.args().size() - ARGS_IN_REGISTER;
    }

    bool alignmentCorrection = m_stackAlignment % 2 == 1;

    if (alignmentCorrection) {
        emit("pushq", SizedRegister{Register::RCX, 8});
    }

    for (int i = call.args().size() - 1; i >= (int)ARGS_IN_REGISTER; i--) {
        std::cerr << i << " " << call.args().size() << std::endl;
        RValue arg = call.args()[i];
        auto type = std::visit([](auto val) { return val.type(); }, arg);
        auto suffix = getSuffix(type->size());

        SizedRegister rax{Register::RAX, 8};
        SizedRegister ax{Register::RAX, type->size()};
        auto optionalReg = rvalueRegister(arg);
        if (optionalReg) {
            emit("pushq", SizedRegister(optionalReg.value(), 8));
        } else {
            emit("mov", suffix, arg, ax);
            emit("pushq", rax);
        }
    }

    if (call.variadic()) {
        emit("movq", Immediate(0, types::LONG), SizedRegister(Register::RAX, 8));
    }

    emit("call", FunctionLabel(call.name()));

    if (alignmentCorrection) {
        emit("popq", SizedRegister{Register::RCX, 8});
    }

    for (size_t i = ARGS_IN_REGISTER; i < call.args().size(); i++) {
        emit("popq", SizedRegister(Register::RCX, 8));
        m_stackAlignment--;
    }

    if (call.destination().type()->size() > 0) {
        SizedRegister rax = {Register::RAX, call.destination().type()->size()};
        auto suffix = getSuffix(rax.size);
        emit("mov", suffix, rax, call.destination());
    }

    for (const auto& saved : savedRegister | std::views::reverse) {
        emit("popq", SizedRegister(saved, 8));
        m_stackAlignment--;
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
        RValue source = cast.source();
        std::visit([&](auto& source) { source.setType(destinationType); }, source);
        if (optionalReg) {
            reg.reg = optionalReg.value();
            emit("mov", suffix, source, reg);
        } else {
            emit("mov", suffix, source, reg);
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
            emit("leaq", DataLabel(literalLabel(literal)), destReg);
        } else {
            emit("leaq", DataLabel(literalLabel(literal)), destReg);
            emit("movq", destReg, addressOf.destination());
        }
    }
}

void X86GenVisitor::simplifyAsm() {
    bool changed = true;
    while (changed) {
        changed = false;
        std::unordered_set<std::string> usedLabels;
        for (size_t i = 0; i < m_instructions.size(); i++) {
            auto& instr = m_instructions[i];
            if (instr.empty())
                continue;

            if ((instr[0] == "movq" || instr[0] == "movl" || instr[0] == "movw" || instr[0] == "movb") &&
                instr.at(1) == instr.at(2)) {
                instr.clear();
                changed = true;
                continue;
            }

            if (i != m_instructions.size() - 1) {
                if (instr[0] == "jmp" && m_instructions[i + 1][0].starts_with(instr[1])) {
                    instr.clear();
                    changed = true;
                    continue;
                }
            }

            if (instr[0].starts_with("j") && instr.size() > 1) {
                usedLabels.insert(instr[1]);
            }
        }

        for (auto& instr : m_instructions) {
            if (instr.empty())
                continue;

            if (instr[0].ends_with(":") && !usedLabels.contains(instr[0].substr(0, instr[0].size() - 1))) {
                instr.clear();
            }
        }

        auto it = std::remove_if(m_instructions.begin(), m_instructions.end(), [](auto vec) { return vec.empty(); });
        if (it != m_instructions.end())
            m_instructions.erase(it);
    }
}

void X86GenVisitor::printAsm() {
    for (const auto& instr : m_instructions) {
        if (instr.empty())
            continue;
        if (!instr[0].ends_with(":"))
            m_out << "\t";
        m_out << instr[0] << "\t";
        for (size_t i = 1; i < instr.size(); i++) {
            if (i != 1)
                m_out << ", ";
            m_out << instr[i];
        }
        m_out << "\n";
    }
}
