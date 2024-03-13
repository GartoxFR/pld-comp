#include "./X86GenVisitor.h"
#include "ir/Instructions.h"

using namespace ir;

// helper type for the visitor
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
// explicit deduction guide
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

void X86GenVisitor::visit(ir::BinaryOp& binaryOp) {
    //TODO: Generate asm

    RValue left = binaryOp.left();
    RValue right = binaryOp.right();

    std::visit(
        overloaded{
            [&](Local& left, Immediate& right) {
                // Left member is a local variable and right is an immediate
            },
            [&](Immediate& left, Local& right) {

            },
            [&](Local& right, Local& left) {},
            [&](Immediate& right, Immediate& left) {},
        },
        left, right
    );
}

void X86GenVisitor::visit(ir::Assignment& assignment) {
    //TODO: Generate asm
}

void X86GenVisitor::visit(ir::BasicJump& jump) {
    //TODO: Generate asm
}
