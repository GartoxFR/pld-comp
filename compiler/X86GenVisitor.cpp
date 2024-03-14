#include "./X86GenVisitor.h"
#include "ir/Instructions.h"
#include <variant>

using namespace ir;

template <typename... Ts>
class overload : Ts... {
    using Ts::operator()...;
};

template <typename... Ts>
overload(Ts...) -> overload<Ts...>;

void X86GenVisitor::visit(ir::Function& function) {
    m_out << ".global " << function.name() << "\n";
    m_out << function.name() << ":";
    std::cout << "    pushq   %rbp\n";
    std::cout << "    movq    %rsp, %rbp\n";
    std::cout << "    subq    $" << function.locals().size() * 4 << ", %rsp\n";

    visit(*function.prologue());
    for (auto& block : function.blocks()) {
        visit(*block);
    }
    visit(*function.epilogue());

    std::cout << "    movq    %rbp, %rsp\n";
    std::cout << "    popq    %rbp\n";
    std::cout << "    ret\n";

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

void X86GenVisitor::visit(ir::BinaryOp& binaryOp) {
    std::string_view instruction;
    switch (binaryOp.operation()) {

        case BinaryOpKind::ADD: instruction = "addl"; break;
        case BinaryOpKind::SUB: instruction = "subl"; break;
        case BinaryOpKind::MUL: instruction = "imull"; break;
        case BinaryOpKind::DIV: break;
    }

    if (std::holds_alternative<Local>(binaryOp.left())) {
        Local left = std::get<Local>(binaryOp.left());
        loadEax(left);
        if (std::holds_alternative<Local>(binaryOp.right())) {
            Local right = std::get<Local>(binaryOp.right());
            m_out << "    " << instruction << "    " << variableLocation(right) << ", %eax\n";
        } else {
            Immediate right = std::get<Immediate>(binaryOp.right());
            m_out << "    " << instruction << "    $" << right.value() << ", %eax\n";
        }
    } else {
        Immediate left = std::get<Immediate>(binaryOp.left());
        loadEax(left);
        if (std::holds_alternative<Local>(binaryOp.right())) {
            Local right = std::get<Local>(binaryOp.right());
            m_out << "    " << instruction << "    " << variableLocation(right) << ", %eax\n";
        } else {
            Immediate right = std::get<Immediate>(binaryOp.right());
            m_out << "    " << instruction << "    $" << right.value() << ", %eax\n";
        }
    }

    saveEax(binaryOp.destination());
}

void X86GenVisitor::visit(ir::UnaryOp& unaryOp) {
    if (std::holds_alternative<Local>(unaryOp.operand())) {
        Local source = std::get<Local>(unaryOp.operand());
        loadEax(source);
    } else {
        Immediate source = std::get<Immediate>(unaryOp.operand());
        loadEax(source);
    }
    m_out << "    negl    %eax\n";
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

void X86GenVisitor::visit(ir::BasicJump& jump) {
    //TODO: Generate asm
    m_out << "    jmp     " << jump.target()->label() << "\n";
}
void X86GenVisitor::visit(ir::ConditionalJump& jump) {
    //TODO: Generate asm
}
