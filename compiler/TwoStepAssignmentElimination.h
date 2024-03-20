#pragma once

#include "BlockLivenessAnalysis.h"
#include "ir/Ir.h"

class TwoStepAssignmentEliminationVisitor : public ir::Visitor {
  public:
    using ir::Visitor::visit;

    TwoStepAssignmentEliminationVisitor(BlockLivenessAnalysis& m_livenessAnalysis) :
        m_livenessAnalysis(m_livenessAnalysis) {}

    void visit(ir::BasicBlock& block) override;
    void visit(ir::BinaryOp& binaryOp) override;
    void visit(ir::UnaryOp& unaryOp) override;
    void visit(ir::Assignment& assignment) override;
    void visit(ir::ConditionalJump& jump) override;
    void visit(ir::Call& call) override;

    void variableUsed(ir::RValue rvalue) {
        if (std::holds_alternative<ir::Local>(rvalue))
            variableUsed(std::get<ir::Local>(rvalue));
    }

    void variableUsed(ir::Local local) { 
        m_workingSet.insert(local); 
        m_potentialTarget.erase(local);
    }

    void variableAssigned(ir::Local& local) { 
        m_workingSet.erase(local); 
        auto it = m_potentialTarget.find(local);
        if (it != m_potentialTarget.end()) {
            local = it->second.first;
            *it->second.second = nullptr;
            m_potentialTarget.erase(local);
            m_changed = true;
        }
    }

    bool changed() const { return m_changed; }

  private:
    LiveSet m_workingSet;
    BlockLivenessAnalysis& m_livenessAnalysis;
    std::unordered_map<ir::Local, std::pair<ir::Local, std::unique_ptr<ir::Instruction>*>> m_potentialTarget;
    std::unique_ptr<ir::Instruction>* m_currentInstruction;
    bool m_changed = false;
};
