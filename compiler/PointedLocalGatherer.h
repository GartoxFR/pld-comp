#pragma once

#include "ir/Visitor.h"

#include <unordered_set>

using PointedLocals = std::unordered_set<ir::Local>;

class PointedLocalsGatherer : public ir::Visitor {
  public:
    using ir::Visitor::visit;

    void visit(ir::AddressOf& instr) override { 
        if (std::holds_alternative<ir::Local>(instr.source())) {
            m_pointedLocals.insert(std::get<ir::Local>(instr.source())); 
        }
    }

  private:
    PointedLocals m_pointedLocals;

    friend PointedLocals computePointedLocals(ir::Function& function);
};

inline PointedLocals computePointedLocals(ir::Function& function) {
    PointedLocalsGatherer gatherer;
    gatherer.visit(function);
    auto locals = std::move(gatherer.m_pointedLocals);
    return locals;
}
