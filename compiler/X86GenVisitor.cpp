#include "./X86GenVisitor.h"
#include "ir/Instructions.h"
#include <variant>

using namespace ir;

static const char* CALL_REGISTER[] = {
    "%edi",
    "%esi",
    "%edx",
    "%ecx",
};

void X86GenVisitor::visit(ir::Function& function) {
    m_out << ".global " << function.name() << "\n";
    m_out << function.name() << ":";
    m_out << "    pushq   %rbp\n";
    m_out << "    movq    %rsp, %rbp\n";
    m_out << "    subq    $" << function.locals().size() * 4 << ", %rsp\n";

    for (size_t i = 0; i < function.argCount(); i++) {
        m_out << "    movl    " << CALL_REGISTER[i] << ", " << variableLocation(Local(i + 1)) << "\n";
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
    loadEax(binaryOp.left());

    if (std::holds_alternative<Local>(binaryOp.right())) {
        Local rightLocal = std::get<Local>(binaryOp.right());
        m_out << "    " << instruction << "    " << variableLocation(rightLocal) << ", %eax\n";
    } else {
        Immediate rightImmediate = std::get<Immediate>(binaryOp.right());
        m_out << "    " << instruction << "    $" << rightImmediate.value() << ", %eax\n";
    }
    saveEax(binaryOp.destination());
}

void X86GenVisitor::emitEq(const ir::BinaryOp& binaryOp) {
    loadEax(binaryOp.left());
    m_out << "    movl    $1, %edx\n";
    if (std::holds_alternative<Local>(binaryOp.right())) {
        Local rightLocal = std::get<Local>(binaryOp.right());
        m_out << "    cmpl    " << variableLocation(rightLocal) << ", %eax\n";
    } else {
        Immediate rightImmediate = std::get<Immediate>(binaryOp.right());
        m_out << "    cmpl    $" << rightImmediate.value() << ", %eax\n";
    }
    m_out << "    movl    $0, %eax\n";
    m_out << "    cmovel  %edx, %eax\n";
    saveEax(binaryOp.destination());
}

void X86GenVisitor::emitNeq(const ir::BinaryOp& binaryOp) {
    loadEax(binaryOp.left());
    m_out << "    movl    $0, %edx\n";
    if (std::holds_alternative<Local>(binaryOp.right())) {
        Local rightLocal = std::get<Local>(binaryOp.right());
        m_out << "    cmpl    " << variableLocation(rightLocal) << ", %eax\n";
    } else {
        Immediate rightImmediate = std::get<Immediate>(binaryOp.right());
        m_out << "    cmpl    $" << rightImmediate.value() << ", %eax\n";
    }
    m_out << "    movl    $1, %eax\n";
    m_out << "    cmovel  %edx, %eax\n";
    saveEax(binaryOp.destination());
}

void X86GenVisitor::emitCmp(std::string_view instruction, const ir::BinaryOp& binaryOp) {
    loadEax(binaryOp.left());
    m_out << "    movl    $1, %edx\n";
    if (std::holds_alternative<Local>(binaryOp.right())) {
        Local rightLocal = std::get<Local>(binaryOp.right());
        m_out << "    cmpl    " << variableLocation(rightLocal) << ", %eax\n";
    } else {
        Immediate rightImmediate = std::get<Immediate>(binaryOp.right());
        m_out << "    cmpl    $" << rightImmediate.value() << ", %eax\n";
    }
    m_out << "    movl    $0, %eax\n";
    m_out << "    " << instruction << " %edx, %eax\n";
    saveEax(binaryOp.destination());
}

void X86GenVisitor::visit(ir::BinaryOp& binaryOp) {
    switch (binaryOp.operation()) {
        case BinaryOpKind::ADD: emitSimpleArithmetic("addl", binaryOp); break;
        case BinaryOpKind::SUB: emitSimpleArithmetic("subl", binaryOp); break;
        case BinaryOpKind::MUL: emitSimpleArithmetic("imull", binaryOp); break;
        case BinaryOpKind::DIV: break;
        case BinaryOpKind::EQ: emitCmp("cmovel ", binaryOp); break;
        case BinaryOpKind::NEQ: emitCmp("cmovnel ", binaryOp); break;
        case BinaryOpKind::CMP_L: emitCmp("cmovll ", binaryOp); break;
        case BinaryOpKind::CMP_G: emitCmp("cmovgl ", binaryOp); break;
        case BinaryOpKind::CMP_LE: emitCmp("cmovlel ", binaryOp); break;
        case BinaryOpKind::CMP_GE: emitCmp("cmovgel ", binaryOp); break;
    }
}

void X86GenVisitor::visit(ir::UnaryOp& unaryOp) {
    loadEax(unaryOp.operand());
    switch (unaryOp.operation()) {
        case UnaryOpKind::MINUS: m_out << "    negl    %eax\n"; break;

        case UnaryOpKind::NOT:
            m_out << "    movl    $1, %edx\n";
            m_out << "    test    %eax, %eax\n";
            m_out << "    movl    $0, %eax\n";
            m_out << "    cmovzl  %edx, %eax\n";
            break;
    }
    saveEax(unaryOp.destination());
}

void X86GenVisitor::visit(ir::Assignment& assignment) {
    if (std::holds_alternative<Local>(assignment.source())) {
        Local source = std::get<Local>(assignment.source());
        loadEax(source);
        saveEax(assignment.destination());
    } else {
        Immediate source = std::get<Immediate>(assignment.source());
        m_out << "    movl    $" << source.value() << ", " << variableLocation(assignment.destination()) << "\n";
    }
}

void X86GenVisitor::visit(ir::BasicJump& jump) { m_out << "    jmp     " << jump.target()->label() << "\n"; }
void X86GenVisitor::visit(ir::ConditionalJump& jump) {
    loadEax(jump.condition());
    m_out << "    test    %eax, %eax\n";
    m_out << "    jne     " << jump.trueTarget()->label() << "\n";
    m_out << "    jmp     " << jump.falseTarget()->label() << "\n";
}

void X86GenVisitor::visit(ir::Call& call) {
    for (size_t i = 0; i < call.args().size(); i++) {
        RValue arg = call.args()[i];
        if (std::holds_alternative<Local>(arg)) {
            Local argLocal = std::get<Local>(arg);
            m_out << "    movl    " << variableLocation(argLocal) << ", " << CALL_REGISTER[i] << "\n";
        } else {
            Immediate argLocal = std::get<Immediate>(arg);
            m_out << "    movl    $" << argLocal.value() << ", " << CALL_REGISTER[i] << "\n";
        }
    }

    m_out << "    call   " << call.name() << "\n";
    saveEax(call.destination());
}
