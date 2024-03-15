#pragma once

#include "IrSymbolTable.h"
#include "generated/ifccBaseVisitor.h"
#include "ir/Function.h"
#include "ir/Ir.h"
#include <vector>

class IrGenVisitor : public ifccBaseVisitor {
  public:
    std::any visitProg(ifccParser::ProgContext* ctx) override;
    std::any visitReturn_stmt(ifccParser::Return_stmtContext* ctx) override;

    std::any visitConst(ifccParser::ConstContext* ctx) override;
    std::any visitVar(ifccParser::VarContext* ctx) override;

    std::any visitBlock(ifccParser::BlockContext *ctx) override;
    std::any visitIf(ifccParser::IfContext *ctx) override;
    std::any visitWhile(ifccParser::WhileContext *ctx) override;

    std::any visitInitializer(ifccParser::InitializerContext* ctx) override;
    std::any visitAssign(ifccParser::AssignContext* ctx) override;

    std::any visitSumOp(ifccParser::SumOpContext* ctx) override;
    std::any visitProductOp(ifccParser::ProductOpContext* ctx) override;
    std::any visitUnarySumOp(ifccParser::UnarySumOpContext* ctx) override;

    std::any visitPar(ifccParser::ParContext* ctx) override { return visit(ctx->expr()); }

    std::any visitBinaryOp(ifccParser::ExprContext* left, ifccParser::ExprContext* right, ir::BinaryOpKind op);
    std::any visitUnaryOp(ifccParser::ExprContext* operand, ir::UnaryOpKind op);

    const auto& functions() const { return m_functions; }

    bool hasErrors() const { return m_symbolTable.hasErrors(); }

  private:
    std::vector<std::unique_ptr<ir::Function>> m_functions;
    ir::Function* m_currentFunction;
    ir::BasicBlock* m_currentBlock;
    IrSymbolTable m_symbolTable;
};
