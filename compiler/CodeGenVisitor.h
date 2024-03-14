#pragma once

#include "SymbolTable.h"
#include "generated/ifccBaseVisitor.h"

class CodeGenVisitor : public ifccBaseVisitor {
  public:
    CodeGenVisitor() : m_nextTemporaries(globalSymbolTable.size() * 4 + 4) {}

    antlrcpp::Any visitProg(ifccParser::ProgContext* ctx) override;
    antlrcpp::Any visitReturn_stmt(ifccParser::Return_stmtContext* ctx) override;

    std::any visitConst(ifccParser::ConstContext* ctx) override;
    std::any visitVar(ifccParser::VarContext* ctx) override;

    std::any visitInitializer(ifccParser::InitializerContext* ctx) override;
    std::any visitAssign(ifccParser::AssignContext* ctx) override;

    std::any visitSumOp(ifccParser::SumOpContext* ctx) override;
    std::any visitProductOp(ifccParser::ProductOpContext* ctx) override;
    std::any visitUnarySumOp(ifccParser::UnarySumOpContext* ctx) override;

    std::any visitPar(ifccParser::ParContext* ctx) override { return visit(ctx->expr()); }

  private:
    void loadVariable(int var);
    void storeVariable(int var);
    int nextTemp() {
        int temp = m_nextTemporaries;
        m_nextTemporaries += 4;
        return temp;
    }

    int m_nextTemporaries;
};
