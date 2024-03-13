#include "./X86GenVisitor.h"
#include "ir/Instructions.h"

using namespace ir;

void X86GenVisitor::visit(ir::Function &function) {
    //TODO: Generate asm
}

void X86GenVisitor::visit(ir::BasicBlock &block) {
    //TODO: Generate asm
}

void X86GenVisitor::visit(ir::BinaryOp& binaryOp) {
    //TODO: Generate asm
}

void X86GenVisitor::visit(ir::Assignment& assignment) {
    //TODO: Generate asm
}

void X86GenVisitor::visit(ir::BasicJump& jump) {
    //TODO: Generate asm
}
void X86GenVisitor::visit(ir::ConditionalJump& jump) {
    //TODO: Generate asm
}
