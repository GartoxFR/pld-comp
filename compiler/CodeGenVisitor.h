#pragma once

#include "generated/ifccBaseVisitor.h"

class CodeGenVisitor : public ifccBaseVisitor {
  public:
    CodeGenVisitor(std::map<std::string, int>&& symbolTable) :
        m_symbolTable(symbolTable), m_nextTemporaries(symbolTable.size() + 4) {}

    antlrcpp::Any visitProg(ifccParser::ProgContext* ctx) override;
    antlrcpp::Any visitReturn_stmt(ifccParser::Return_stmtContext* ctx) override;

    std::any visitConst(ifccParser::ConstContext* ctx) override;
    std::any visitVar(ifccParser::VarContext* ctx) override;

    std::any visitAssign_stmt(ifccParser::Assign_stmtContext* ctx) override;

    std::any visitSumOp(ifccParser::SumOpContext* ctx) override;
    std::any visitProductOp(ifccParser::ProductOpContext* ctx) override;
    std::any visitUnarySumOp(ifccParser::UnarySumOpContext* ctx) override;

    std::any visitPar(ifccParser::ParContext* ctx) override {
        return visit(ctx->expr());
    }

  private:
    void loadVariable(int var);
    void storeVariable(int var);
    int nextTemp() {
        int temp = m_nextTemporaries;
        m_nextTemporaries += 4;
        return temp;
    }

    std::map<std::string, int> m_symbolTable;
    int m_nextTemporaries;
};
