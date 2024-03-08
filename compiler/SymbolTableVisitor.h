#pragma once

#include "SymbolTable.h"
#include "generated/ifccBaseVisitor.h"
#include <string>

class SymbolTableVisitor : public ifccBaseVisitor {
  public:
    SymbolTableVisitor(SymbolTable& symbolTable) : m_symbolTable(symbolTable) {}

    std::any visitVar(ifccParser::VarContext* context) override;
    std::any visitDeclare_stmt(ifccParser::Declare_stmtContext* context) override;
    std::any visitAssign_stmt(ifccParser::Assign_stmtContext* ctx) override;

    bool createSymbolTable(antlr4::tree::ParseTree* axiom) {
        this->visit(axiom);

        for (auto& [ident, id] : m_symbolTable) {
            if (m_usedSymbols.find(ident) == m_usedSymbols.end()) {
                std::cerr << "Warning: Variable " << ident << " déclarée mais non utilisée" << std::endl;
            }
        }

        return success;
    }

  private:

    SymbolTable& m_symbolTable;
    std::set<std::string> m_usedSymbols;
    bool success = true;
};
