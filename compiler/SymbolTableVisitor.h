#pragma once

#include "SymbolTable.h"
#include "generated/ifccBaseVisitor.h"

class SymbolTableVisitor : public ifccBaseVisitor {
  public:
    std::any visitVar(ifccParser::VarContext* context) override;
    std::any visitDeclare_stmt(ifccParser::Declare_stmtContext* context) override;
    std::any visitAssign_stmt(ifccParser::Assign_stmtContext* ctx) override;

    bool createSymbolTable(antlr4::tree::ParseTree* axiom) {
        this->visit(axiom);

        for (auto& [ident, id] : globalSymbolTable) {
            if (m_usedSymbols.find(ident) == m_usedSymbols.end()) {
                std::cerr << "Warning: Variable " << ident << " déclarée mais non utilisée" << std::endl;
            }
        }

        return success;
    }

  private:
    std::set<Ident> m_usedSymbols;
    bool success = true;
};
