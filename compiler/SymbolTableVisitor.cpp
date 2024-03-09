#include "./SymbolTableVisitor.h"
#include <iostream>

std::any SymbolTableVisitor::visitVar(ifccParser::VarContext* context) {
    Ident ident = make_ident(context->IDENT());

    if (!globalSymbolTable.contains(ident)) {
        std::cerr << "Erreur: Variable " << ident << " non déclarée" << std::endl;

        success = false;
        return 0;
    }

    m_readSymbols.insert(ident);

    return 0;
}

std::any SymbolTableVisitor::visitDeclare_stmt(ifccParser::Declare_stmtContext* context) {
    for (auto initializer : context->idents) {
        Ident ident = make_ident(initializer->IDENT());

        if (globalSymbolTable.contains(ident)) {
            std::cerr << "Erreur: Variable " << ident << " déjà déclarée" << std::endl;

            success = false;
            return 0;
        }

        globalSymbolTable.declare(ident);

        // Variable is initialized inline so it's written at least once
        if (initializer->expr()) {
            m_writtenSymbols.insert(ident);
        }
    }
    visitChildren(context);
    return 0;
}

std::any SymbolTableVisitor::visitAssign_stmt(ifccParser::Assign_stmtContext* ctx) {
    Ident ident = make_ident(ctx->IDENT());

    if (!globalSymbolTable.contains(ident)) {
        std::cerr << "Erreur: Variable " << ident << " non déclarée" << std::endl;

        success = false;
        return 0;
    }

    m_writtenSymbols.insert(ident);
    return 0;
}
