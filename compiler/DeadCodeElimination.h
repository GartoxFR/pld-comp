#pragma once

#include "ir/Ir.h"
#include "ir/Visitor.h"
#include <unordered_map>
#include <unordered_set>
#include <variant>

class BlockLivenessAnalysis : public ir::Visitor {
  public:
    using ir::Visitor::visit;

    void visit(ir::Function& function) override;
    void visit(ir::BasicBlock& block) override;
    void visit(ir::BinaryOp& binaryOp) override;
    void visit(ir::UnaryOp& unaryOp) override;
    void visit(ir::Assignment& assignment) override;
    void visit(ir::ConditionalJump& jump) override;
    void visit(ir::Call& call) override;

    std::unordered_map<ir::BasicBlock*, std::unordered_set<ir::Local>> blocksOutputLiveness() {
        std::unordered_map<ir::BasicBlock*, std::unordered_set<ir::Local>> res;
        for (auto& [block, pair] : m_liveMap) {
            res.insert({block, pair.second});
        }
        return res;
    }

  private:
    using LiveSet = std::unordered_set<ir::Local>;

    std::unordered_map<ir::BasicBlock*, std::pair<LiveSet, LiveSet>> m_liveMap;
    std::unordered_map<ir::BasicBlock*, std::vector<ir::BasicBlock*>> m_dependanceMap;

    LiveSet m_workingSet;

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

    bool propagate(ir::BasicBlock* source, ir::BasicBlock* target) {
        LiveSet& sourceSet = m_liveMap[source].first;
        LiveSet& targetSet = m_liveMap[target].second;

        bool changed = false;
        for (auto local : sourceSet) {
            changed |= targetSet.insert(local).second;
        }

        return changed;
    }

    bool flushBlockInput(ir::BasicBlock* block) {
        LiveSet& inputSet = m_liveMap[block].first;

        bool changed = false;
        for (auto local : m_workingSet) {
            changed |= inputSet.insert(local).second;
        }

        m_workingSet.clear();
        return changed;
    }
};

class DeadCodeElimination : public ir::Visitor {
  public:
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
    std::unordered_map<ir::BasicBlock*, std::unordered_set<ir::Local>> m_blocksOutputs;
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
