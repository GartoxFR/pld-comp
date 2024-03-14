#pragma once

#include "SymbolTable.h"
#include "generated/ifccBaseVisitor.h"

class SymbolTableVisitor : public ifccBaseVisitor {
  public:
    std::any visitVar(ifccParser::VarContext* context) override;
    std::any visitDeclare_stmt(ifccParser::Declare_stmtContext* context) override;
    std::any visitAssign(ifccParser::AssignContext* ctx) override;

    bool createSymbolTable(antlr4::tree::ParseTree* axiom) {
        this->visit(axiom);

        for (auto& [ident, id] : globalSymbolTable) {
            bool read = m_readSymbols.contains(ident);
            bool written = m_writtenSymbols.contains(ident);
            if (!read && !written) {
                std::cerr << "Warning: Variable " << ident << " déclarée mais non utilisée" << std::endl;
            } else if (!read) {
                std::cerr << "Warning: Variable " << ident << " écrite mais jamais lue" << std::endl;
            } else if (!written) {
                std::cerr << "Warning: Variable " << ident << " lue mais jamais initialisée" << std::endl;
            }
        }

        return success;
    }

  private:
    std::unordered_set<Ident> m_readSymbols;
    std::unordered_set<Ident> m_writtenSymbols;
    bool success = true;
};
