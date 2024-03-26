#pragma once

#include "BlockDependance.h"
#include "InterferenceGraph.h"
#include "ir/Ir.h"
#include <unordered_set>

using LiveSet = std::unordered_set<ir::Local>;
using BlockLivenessAnalysis = std::unordered_map<ir::BasicBlock*, std::pair<LiveSet, LiveSet>>;

using LocalsUsedThroughCalls = std::unordered_map<const ir::Instruction*, std::pair<LiveSet, LiveSet>>;

class BlockLivenessAnalysisVisitor : public ir::Visitor {
  public:
    using ir::Visitor::visit;

    BlockLivenessAnalysisVisitor(
        DependanceMap& dependanceMap, InterferenceGraph* interferenceGraph = nullptr,
        LocalsUsedThroughCalls* calls = nullptr
    ) :
        m_dependanceMap(dependanceMap),
        m_interferenceGraph(interferenceGraph), m_calls(calls) {}

    void visit(ir::Function& function) override;
    void visit(ir::BasicBlock& block) override;
    void visit(ir::BinaryOp& binaryOp) override;
    void visit(ir::UnaryOp& unaryOp) override;
    void visit(ir::Assignment& assignment) override;
    void visit(ir::ConditionalJump& jump) override;
    void visit(ir::Call& call) override;
    void visit(ir::Cast& cast) override;
    void visit(ir::PointerRead& read) override;
    void visit(ir::PointerWrite& write) override;
    void visit(ir::AddressOf& address) override;

    auto& blocksLiveness() { return m_liveMap; }

  private:
    BlockLivenessAnalysis m_liveMap;
    DependanceMap& m_dependanceMap;
    InterferenceGraph* m_interferenceGraph;
    LocalsUsedThroughCalls* m_calls;

    LiveSet m_workingSet;

    void setLive(ir::RValue rvalue) {
        if (std::holds_alternative<ir::Local>(rvalue))
            setLive(std::get<ir::Local>(rvalue));
    }

    void unsetLive(ir::RValue rvalue) {
        if (std::holds_alternative<ir::Local>(rvalue))
            unsetLive(std::get<ir::Local>(rvalue));
    }

    void setLive(ir::Local local) {
        if (m_workingSet.insert(local).second && m_interferenceGraph) {
            for (const auto& other : m_workingSet) {
                m_interferenceGraph->addInterference(local.id(), other.id());
            }
        }
    }

    void unsetLive(ir::Local local) { m_workingSet.erase(local); }

    bool propagate(ir::BasicBlock* source, ir::BasicBlock* target) {
        LiveSet& sourceSet = m_liveMap[source].first;
        LiveSet& targetSet = m_liveMap[target].second;

        bool changed = false;
        for (auto local : sourceSet) {
            bool inserted = targetSet.insert(local).second;
            changed |= inserted;
            if (inserted && m_interferenceGraph) {
                for (const auto& other : targetSet) {
                    m_interferenceGraph->addInterference(local.id(), other.id());
                }
            }
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

inline BlockLivenessAnalysis computeBlockLivenessAnalysis(
    ir::Function& function, DependanceMap& dependanceMap, InterferenceGraph* interferenceGraph = nullptr,
    LocalsUsedThroughCalls* calls = nullptr
) {
    BlockLivenessAnalysisVisitor visitor{dependanceMap, interferenceGraph, calls};
    visitor.visit(function);

    auto analysis = std::move(visitor.blocksLiveness());
    return analysis;
}
