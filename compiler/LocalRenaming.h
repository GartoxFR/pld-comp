#pragma once
#include "ir/Ir.h"
#include <set>
#include <unordered_map>

class LocalRenamingVisitor : public ir::Visitor {

  public:
    using ir::Visitor::visit;

    void visit(ir::Function& function) override;
    void visit(ir::BinaryOp& binaryOp) override;
    void visit(ir::UnaryOp& unaryOp) override;
    void visit(ir::Assignment& assignment) override;
    void visit(ir::ConditionalJump& jump) override;
    void visit(ir::Call& call) override;

  private:
    std::unordered_map<ir::Local, ir::Local> m_translationTable;

    void tryRename(ir::RValue& local) {
        if (std::holds_alternative<ir::Local>(local))
            local = m_translationTable.at(std::get<ir::Local>(local));
    }
    void tryRename(ir::Local& local) { local = m_translationTable.at(local); }
};

class LocalUsageVisitor : public ir::Visitor {
  public:
    using ir::Visitor::visit;

    void visit(ir::BinaryOp& binaryOp) override;
    void visit(ir::UnaryOp& unaryOp) override;
    void visit(ir::Assignment& assignment) override;
    void visit(ir::ConditionalJump& jump) override;
    void visit(ir::Call& call) override;

    auto& usedLocal() { return m_usedLocal;}

  private:
    std::set<ir::Local> m_usedLocal;

    void setUsed(const ir::RValue& local) {
        if (std::holds_alternative<ir::Local>(local))
            m_usedLocal.insert(std::get<ir::Local>(local));
    }
    void setUsed(const ir::Local& local) { m_usedLocal.insert(local); }
};
