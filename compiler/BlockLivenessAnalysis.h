#pragma once

#include "BlockDependance.h"
#include "ir/Ir.h"
#include <unordered_set>

using LiveSet = std::unordered_set<ir::Local>;
using BlockLivenessAnalysis = std::unordered_map<ir::BasicBlock*, std::pair<LiveSet, LiveSet>>;

class BlockLivenessAnalysisVisitor : public ir::Visitor {
  public:
    using ir::Visitor::visit;

    BlockLivenessAnalysisVisitor(DependanceMap& dependanceMap) : m_dependanceMap(dependanceMap) {}

    void visit(ir::Function& function) override;
    void visit(ir::BasicBlock& block) override;
    void visit(ir::BinaryOp& binaryOp) override;
    void visit(ir::UnaryOp& unaryOp) override;
    void visit(ir::Assignment& assignment) override;
    void visit(ir::ConditionalJump& jump) override;
    void visit(ir::Call& call) override;

    auto& blocksLiveness() {
        return m_liveMap;
    }

  private:

    BlockLivenessAnalysis m_liveMap;
    DependanceMap& m_dependanceMap;

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

inline BlockLivenessAnalysis computeBlockLivenessAnalysis(ir::Function& function, DependanceMap& dependanceMap) {
    BlockLivenessAnalysisVisitor visitor{dependanceMap};
    visitor.visit(function);

    auto analysis = std::move(visitor.blocksLiveness());
    return analysis;
}
