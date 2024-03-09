#include "./SymbolTableVisitor.h"
#include <iostream>

std::any SymbolTableVisitor::visitVar(ifccParser::VarContext* context) {
    Ident ident = m_symbolTable.toIdent(context->IDENT()->getText());

    if (!m_symbolTable.contains(ident)) {
        std::cerr << "Erreur: Variable " << ident << " non déclarée" << std::endl;

        success = false;
        return 0;
    }

    m_usedSymbols.insert(ident);

    return 0;
}

std::any SymbolTableVisitor::visitDeclare_stmt(ifccParser::Declare_stmtContext* context) {
    for (auto initializer : context->idents) {
        std::string ident = initializer->IDENT()->getText();

        if (m_symbolTable.contains(ident)) {
            std::cerr << "Erreur: Variable " << ident << " déjà déclarée" << std::endl;

            success = false;
            return 0;
        }

        m_symbolTable.declare(std::move(ident));
    }
    return 0;
}

std::any SymbolTableVisitor::visitAssign_stmt(ifccParser::Assign_stmtContext* ctx) {
    Ident ident = m_symbolTable.toIdent(ctx->IDENT()->getText());

    if (!m_symbolTable.contains(ident)) {
        std::cerr << "Erreur: Variable " << ident << " non déclarée" << std::endl;

        success = false;
        return 0;
    }

    m_usedSymbols.insert(std::move(ident));
    return 0;
}
