#include "./SymbolTableVisitor.h"
#include <iostream>

std::any SymbolTableVisitor::visitVar(ifccParser::VarContext* context) {
    std::string ident = context->IDENT()->getText();

    if (m_symbols.find(ident) == m_symbols.end()) {
        std::cerr << "Erreur: Variable " << ident << " non déclarée"
                  << std::endl;

        success = false;
        return 0;
    }

    m_usedSymbols.insert(std::move(ident));

    return 0;
}

std::any SymbolTableVisitor::visitDeclare_stmt(
    ifccParser::Declare_stmtContext* context) {
    std::string ident = context->IDENT()->getText();

    if (m_symbols.find(ident) != m_symbols.end()) {
        std::cerr << "Erreur: Variable " << ident << " déjà déclarée"
                  << std::endl;

        success = false;
        return 0;
    }

    int id = m_nextId;
    m_nextId += 4;

    m_symbols.insert({std::move(ident), id});

    return 0;
}

std::any SymbolTableVisitor::visitAssign_stmt(ifccParser::Assign_stmtContext *ctx) {
    std::string ident = ctx->IDENT()->getText();

    if (m_symbols.find(ident) == m_symbols.end()) {
        std::cerr << "Erreur: Variable " << ident << " non déclarée"
                  << std::endl;

        success = false;
        return 0;
    }

    m_usedSymbols.insert(std::move(ident));
    return 0;
}
