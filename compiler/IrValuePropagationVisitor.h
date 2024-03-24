#pragma once

#include "PointedLocalGatherer.h"
#include "ir/Instructions.h"
#include "ir/Ir.h"
#include <iostream>
#include <unordered_map>

class GlobalValuePropagationVisitor : public ir::Visitor {
  public:
    GlobalValuePropagationVisitor(PointedLocals& pointedLocals) : m_pointedLocals(pointedLocals) {}
    using ir::Visitor::visit;
    void visit(ir::Function& function) override;
    void visit(ir::BinaryOp& binaryOp) override;
    void visit(ir::UnaryOp& unaryOp) override;
    void visit(ir::Assignment& assignment) override;
    void visit(ir::Call& call) override;
    void visit(ir::BasicJump& jump) override;
    void visit(ir::ConditionalJump& jump) override;
    void visit(ir::Cast& cast) override;
    void visit(ir::PointerRead& read) override;
    void visit(ir::AddressOf& address) override;

    std::unordered_map<const ir::BasicBlock*, std::unordered_map<ir::Local, ir::RValue>> mappings() {
        std::unordered_map<const ir::BasicBlock*, std::unordered_map<ir::Local, ir::RValue>> res;
        for (auto& [block, pair] : m_localMappings) {
            for (auto& [local, rvalue] : pair.first) {
                if (rvalue.has_value())
                    res[block].insert({local, rvalue.value()});
            }
        }

        return res;
    }

  private:
    using LocalMapping = std::unordered_map<ir::Local, std::optional<ir::RValue>>;

    std::unordered_map<const ir::BasicBlock*, std::pair<LocalMapping, LocalMapping>> m_localMappings;
    LocalMapping m_workingMapping;
    std::vector<ir::BasicBlock*> m_toVisit;
    ir::BasicBlock* m_currentBlock;
    PointedLocals& m_pointedLocals;

    void propagate(ir::BasicBlock* target) {
        LocalMapping& sourceSet = m_localMappings[m_currentBlock].second;
        LocalMapping& targetSet = m_localMappings[target].first;

        bool changed = false;
        for (auto& [local, rvalue] : sourceSet) {
            auto it = targetSet.find(local);
            if (it == targetSet.end()) {
                targetSet.insert({local, rvalue});
                changed = true;
            } else {
                if (it->second != rvalue) {
                    it->second = std::nullopt;
                    changed = true;
                }
            }
        }

        if (changed) {
            m_toVisit.push_back(target);
        }
    }

    bool flushBlockOutput() {
        LocalMapping& targetSet = m_localMappings[m_currentBlock].second;

        bool changed = false;
        for (auto& [local, rvalue] : m_workingMapping) {
            auto it = targetSet.find(local);
            if (it == targetSet.end()) {
                targetSet.insert({local, rvalue});
                changed = true;
            } else {
                if (it->second != rvalue) {
                    it->second = std::nullopt;
                    changed = true;
                }
            }
        }

        m_workingMapping.clear();
        return changed;
    }

    void setMapping(ir::Local local, ir::RValue value) {
        if (m_pointedLocals.contains(local) ||
            (std::holds_alternative<ir::Local>(value) && m_pointedLocals.contains(std::get<ir::Local>(value)))) {
            return;
        }
        m_workingMapping.insert_or_assign(std::move(local), std::move(value));
    }

    void setNotConstant(ir::Local local) { m_workingMapping.insert_or_assign(std::move(local), std::nullopt); }
};

class IrValuePropagationVisitor : public ir::Visitor {
  public:
    IrValuePropagationVisitor(PointedLocals& pointedLocals) : m_pointedLocals(pointedLocals) {}

    using ir::Visitor::visit;
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

    void invalidateLocal(ir::Local local) {
        m_knownValues.erase(local);
        for (auto it = m_knownValues.begin(); it != m_knownValues.end();) {
            if (ir::RValue(local) == it->second)
                it = m_knownValues.erase(it);
            else
                ++it;
        }
    }

    void trySubstitute(ir::RValue& rvalue) {
        if (std::holds_alternative<ir::Local>(rvalue)) {
            ir::Local local = std::get<ir::Local>(rvalue);
            auto it = m_knownValues.find(local);
            if (it != m_knownValues.end()) {
                rvalue = it->second;
                m_changed = true;
            }
        }
    }

    void setSubstitution(ir::Local local, ir::RValue rvalue) {
        if (m_pointedLocals.contains(local) ||
            (std::holds_alternative<ir::Local>(rvalue) && m_pointedLocals.contains(std::get<ir::Local>(rvalue)))) {
            return;
        }

        m_knownValues.insert_or_assign(std::move(local), std::move(rvalue));
    }

    bool changed() const { return m_changed; }

  private:
    std::unordered_map<ir::Local, ir::RValue> m_knownValues;
    std::unordered_map<const ir::BasicBlock*, std::unordered_map<ir::Local, ir::RValue>> m_earlyMappings;
    PointedLocals& m_pointedLocals;
    bool m_changed = false;
};
