
#include "TwoStepAssignmentElimination.h"
#include <ranges>
#include <algorithm>
#include <variant>

using namespace ir;
void TwoStepAssignmentEliminationVisitor::visit(ir::BasicBlock& block) {
    m_workingSet = m_livenessAnalysis[&block].second;
    m_potentialTarget.clear();

    if (block.terminator()) {
        visitBase(*block.terminator());
    }

    for (auto& instr : block.instructions() | std::views::reverse) {
        m_currentInstruction = &instr;
        visitBase(*instr);
    }

    auto removeIt = std::remove(block.instructions().begin(), block.instructions().end(), nullptr);
    block.instructions().erase(removeIt, block.instructions().end());
}

void TwoStepAssignmentEliminationVisitor::visit(ir::BinaryOp& binaryOp) {
    variableAssigned(binaryOp.destination());
    variableUsed(binaryOp.left());
    variableUsed(binaryOp.right());
}
void TwoStepAssignmentEliminationVisitor::visit(ir::UnaryOp& unaryOp) {
    variableAssigned(unaryOp.destination());
    variableUsed(unaryOp.operand());
}
void TwoStepAssignmentEliminationVisitor::visit(ir::Assignment& assignment) {
    if (std::holds_alternative<Local>(assignment.source())) {
        Local source = std::get<Local>(assignment.source());
        bool isTarget = !m_workingSet.contains(source) && !m_pointedLocals.contains(source);
        variableAssigned(assignment.destination());
        variableUsed(source);
        if (isTarget) {
            m_potentialTarget.insert({source, {assignment.destination(), m_currentInstruction}});
        }
    } else {
        variableAssigned(assignment.destination());
        variableUsed(assignment.source());
    }
}
void TwoStepAssignmentEliminationVisitor::visit(ir::ConditionalJump& jump) {
    variableUsed(jump.condition());
}
void TwoStepAssignmentEliminationVisitor::visit(ir::Call& call) {
    variableAssigned(call.destination());
    for (auto& arg : call.args()) {
        variableUsed(arg);
    }
}

void TwoStepAssignmentEliminationVisitor::visit(ir::Cast& cast) {
    variableAssigned(cast.destination());
    variableUsed(cast.source());
}

void TwoStepAssignmentEliminationVisitor::visit(ir::PointerRead& read) {
    variableAssigned(read.destination());
    variableUsed(read.address());
}
void TwoStepAssignmentEliminationVisitor::visit(ir::PointerWrite& write) {
    variableUsed(write.address());
    variableUsed(write.source());
}
void TwoStepAssignmentEliminationVisitor::visit(ir::AddressOf& address) {
    variableAssigned(address.destination());
    if (std::holds_alternative<ir::Local>(address.source())) {
        variableUsed(std::get<ir::Local>(address.source()));
    }
}
