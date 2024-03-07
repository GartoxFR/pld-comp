#pragma once

#include "generated/ifccBaseVisitor.h"
#include <map>
#include <string>

class SymbolTableVisitor : public ifccBaseVisitor {
  public:
    std::any visitVar(ifccParser::VarContext* context) override;
    std::any
    visitDeclare_stmt(ifccParser::Declare_stmtContext* context) override;
    std::any visitAssign_stmt(ifccParser::Assign_stmtContext* ctx) override;


    bool createSymbolTable(antlr4::tree::ParseTree* axiom) {
        this->visit(axiom);

        for (auto& [ident, id] : m_symbols) {
            if (m_usedSymbols.find(ident) == m_usedSymbols.end()) {
                std::cerr << "Warning: Variable " << ident
                          << " déclarée mais non utilisée" << std::endl;
            }
        }

        return success;
    }

    std::map<std::string, int>&& getSymbolTable() {
        return std::move(m_symbols);
    }

  private:
    std::map<std::string, int> m_symbols;
    std::set<std::string> m_usedSymbols;
    int m_nextId = 4;
    bool success = true;
};
