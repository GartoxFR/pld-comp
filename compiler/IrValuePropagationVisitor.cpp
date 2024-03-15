#include "IrValuePropagationVisitor.h"

void IrValuePropagationVisitor::visit(ir::BasicBlock& block) {
    clearSubstitutions();
    for (auto& instr : block.instructions()) {
        visitBase(*instr);
    }

    if (block.terminator()) {
        visitBase(*block.terminator());
    }
}
void IrValuePropagationVisitor::visit(ir::BinaryOp& binaryOp) {
    trySubstitute(binaryOp.left());
    trySubstitute(binaryOp.right());
    invalidateLocal(binaryOp.destination());
}
void IrValuePropagationVisitor::visit(ir::UnaryOp& unaryOp) {
    trySubstitute(unaryOp.operand());
    invalidateLocal(unaryOp.destination());
}
void IrValuePropagationVisitor::visit(ir::Assignment& assignment) {
    trySubstitute(assignment.source());
    setSubstitution(assignment.destination(), assignment.source());
}

void IrValuePropagationVisitor::visit(ir::ConditionalJump& jump) {
    trySubstitute(jump.condition());
}
void IrValuePropagationVisitor::visit(ir::Call& call) {
    for (auto& arg : call.args()) {
        trySubstitute(arg);
    }
}
