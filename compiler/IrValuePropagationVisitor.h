#pragma once

#include "ir/Instructions.h"
#include "ir/Ir.h"
#include <unordered_map>

class IrValuePropagationVisitor : public ir::Visitor {
  public:
    using ir::Visitor::visit;
    void visit(ir::BasicBlock& block) override;
    void visit(ir::BinaryOp& binaryOp) override;
    void visit(ir::UnaryOp& unaryOp) override;
    void visit(ir::Assignment& assignment) override;
    void visit(ir::ConditionalJump& jump) override;
    void visit(ir::Call& call) override;

    void invalidateLocal(ir::Local local) { m_knownValues.erase(local); }

    void clearSubstitutions() {m_knownValues.clear(); }

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
        m_knownValues.insert({std::move(local), std::move(rvalue)}).first->second = rvalue;
    }

    bool changed() const {
        return m_changed;
    }

  private:
    std::unordered_map<ir::Local, ir::RValue> m_knownValues;
    bool m_changed = false;
};
