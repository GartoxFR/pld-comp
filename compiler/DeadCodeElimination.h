#pragma once

#include "BlockLivenessAnalysis.h"
#include "ir/Ir.h"
#include "ir/Visitor.h"
#include <unordered_map>
#include <unordered_set>
#include <variant>

class DeadCodeElimination : public ir::Visitor {
  public:
    DeadCodeElimination(BlockLivenessAnalysis& analysis) : m_blocksLivenessAnalysis(analysis) {}
    void visit(ir::Function& function) override;
    void visit(ir::BasicBlock& block) override;
    void visit(ir::BinaryOp& binaryOp) override;
    void visit(ir::UnaryOp& unaryOp) override;
    void visit(ir::Assignment& assignment) override;
    void visit(ir::ConditionalJump& jump) override;
    void visit(ir::Call& call) override;

    bool changed() const { return m_changed; }

  private:
    std::unordered_set<ir::Local> m_workingSet;
    std::unique_ptr<ir::Instruction>* m_currentInstruction;
    BlockLivenessAnalysis& m_blocksLivenessAnalysis;
    bool m_changed = false;

    void setLive(ir::RValue rvalue) {
        if (std::holds_alternative<ir::Local>(rvalue))
            m_workingSet.insert(std::get<ir::Local>(rvalue));
    }

    void unsetLive(ir::RValue rvalue) {
        if (std::holds_alternative<ir::Local>(rvalue))
            m_workingSet.erase(std::get<ir::Local>(rvalue));
    }

    void setLive(ir::Local local) { m_workingSet.insert(local); }

    void unsetLive(ir::Local local) { m_workingSet.erase(local); }

    bool tryDrop(ir::Local local) {
        if (!m_workingSet.contains(local)) {
            *m_currentInstruction = nullptr;
            m_changed = true;
            return true;
        }
        return false;
    }
};
