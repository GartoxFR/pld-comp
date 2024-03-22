#include "LocalRenaming.h"

using namespace ir;

void LocalRenamingVisitor::visit(ir::Function& function) {
    LocalUsageVisitor usageVisitor;
    for (uint32_t i = 1; i <= function.argCount(); i++) {
        usageVisitor.usedLocal().insert(Local{i, function.locals()[i].type()});
    }
    usageVisitor.visit(function);

    auto oldLocals = std::move(function.locals());
    function.locals().clear();
    for (auto& usedLocal : usageVisitor.usedLocal()) {
        auto& oldInfo = oldLocals[usedLocal.id()];
        Local newLocal = [&]() {
            if (oldInfo.name()) {
                return function.newLocal(std::move(oldInfo.name().value()), oldInfo.type());
            } else {
                return function.newLocal(oldInfo.type());
            }
        }();

        m_translationTable.insert({usedLocal, newLocal});
    }

    visit(*function.prologue());
    for (auto& block : function.blocks()) {
        visit(*block);
    }
    visit(*function.epilogue());
}

void LocalRenamingVisitor::visit(ir::BinaryOp& binaryOp) {
    tryRename(binaryOp.destination());
    tryRename(binaryOp.left());
    tryRename(binaryOp.right());
}
void LocalRenamingVisitor::visit(ir::UnaryOp& unaryOp) {
    tryRename(unaryOp.destination());
    tryRename(unaryOp.operand());
}
void LocalRenamingVisitor::visit(ir::Assignment& assignment) {
    tryRename(assignment.destination());
    tryRename(assignment.source());
}
void LocalRenamingVisitor::visit(ir::ConditionalJump& jump) { tryRename(jump.condition()); }
void LocalRenamingVisitor::visit(ir::Call& call) {
    tryRename(call.destination());
    for (auto& arg : call.args()) {
        tryRename(arg);
    }
}
void LocalRenamingVisitor::visit(ir::Cast& cast) {
    tryRename(cast.source());
    tryRename(cast.destination());
}

void LocalUsageVisitor::visit(ir::BinaryOp& binaryOp) {
    setUsed(binaryOp.destination());
    setUsed(binaryOp.left());
    setUsed(binaryOp.right());
}
void LocalUsageVisitor::visit(ir::UnaryOp& unaryOp) {
    setUsed(unaryOp.destination());
    setUsed(unaryOp.operand());
}
void LocalUsageVisitor::visit(ir::Assignment& assignment) {
    setUsed(assignment.destination());
    setUsed(assignment.source());
}
void LocalUsageVisitor::visit(ir::ConditionalJump& jump) { setUsed(jump.condition()); }
void LocalUsageVisitor::visit(ir::Call& call) {
    setUsed(call.destination());
    for (auto& arg : call.args()) {
        setUsed(arg);
    }
}

void LocalUsageVisitor::visit(ir::Cast& cast) {
    setUsed(cast.source());
    setUsed(cast.destination());
}
