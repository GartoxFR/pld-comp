#pragma once

#include "IrSymbolTable.h"
#include "generated/ifccBaseVisitor.h"
#include "generated/ifccParser.h"
#include "ir/Function.h"
#include "ir/Ir.h"
#include <vector>

class IrGenVisitor : public ifccBaseVisitor {
  public:
    IrGenVisitor();
    std::any visitFunction(ifccParser::FunctionContext *ctx) override;
    std::any visitReturn_stmt(ifccParser::Return_stmtContext* ctx) override;
    std::any visitBreak(ifccParser::BreakContext *ctx) override;
    std::any visitContinue(ifccParser::ContinueContext *ctx) override;

    std::any visitConst(ifccParser::ConstContext* ctx) override;
    std::any visitCharLiteral(ifccParser::CharLiteralContext *ctx) override;
    std::any visitStringLiteral(ifccParser::StringLiteralContext *ctx) override;
    std::any visitLvalueExpr(ifccParser::LvalueExprContext *ctx) override;

    std::any visitBlock(ifccParser::BlockContext *ctx) override;
    std::any visitIf(ifccParser::IfContext *ctx) override;
    std::any visitWhile(ifccParser::WhileContext *ctx) override;

    std::any visitDeclare_stmt(ifccParser::Declare_stmtContext *ctx) override;
    std::any visitAssign(ifccParser::AssignContext* ctx) override;

    std::any visitSumOp(ifccParser::SumOpContext* ctx) override;
    std::any visitProductOp(ifccParser::ProductOpContext* ctx) override;
    std::any visitCmpOp(ifccParser::CmpOpContext *ctx) override;
    std::any visitEqOp(ifccParser::EqOpContext *ctx) override;
    std::any visitUnaryOp(ifccParser::UnaryOpContext* ctx) override;
    std::any visitUnarySumOp(ifccParser::UnarySumOpContext* ctx) override;
    std::any visitCall(ifccParser::CallContext *ctx) override;

    std::any visitBitAnd(ifccParser::BitAndContext *ctx) override;
    std::any visitBitXor(ifccParser::BitXorContext *ctx) override;
    std::any visitBitOr(ifccParser::BitOrContext *ctx) override;

    std::any visitPreIncrDecrOp(ifccParser::PreIncrDecrOpContext *ctx) override;
    std::any visitPostIncrDecrOp(ifccParser::PostIncrDecrOpContext *ctx) override;

    std::any visitLogicalOr(ifccParser::LogicalOrContext *ctx) override;
    std::any visitLogicalAnd(ifccParser::LogicalAndContext *ctx) override;

    std::any visitPar(ifccParser::ParContext* ctx) override { return visit(ctx->expr()); }
    std::any visitLvaluePar(ifccParser::LvalueParContext *ctx) override { return visit(ctx->lvalue()); }

    std::any visitBinaryOp(ifccParser::ExprContext* left, ifccParser::ExprContext* right, ir::BinaryOpKind op, bool comp = false);
    ir::Local visitPointerBinaryOp(ir::Local left, ir::Local right, ir::BinaryOpKind op);
    std::any visitUnaryOp(ifccParser::ExprContext* operand, ir::UnaryOpKind op, bool comp = false);

    std::any visitSimpleType(ifccParser::SimpleTypeContext *ctx) override;
    std::any visitPointerType(ifccParser::PointerTypeContext *ctx) override;

    std::any visitLvalueVar(ifccParser::LvalueVarContext *ctx) override;
    std::any visitLvalueDeref(ifccParser::LvalueDerefContext *ctx) override;
    std::any visitLvalueIndex(ifccParser::LvalueIndexContext *ctx) override;
    std::any visitAddressOf(ifccParser::AddressOfContext *ctx) override;

    ir::Local emitCast(ir::Local source, const Type* targetType);

    const auto& functions() const { return m_functions; }

    bool hasErrors() const { return m_symbolTable.hasErrors() || m_error; }

  private:
    std::vector<std::unique_ptr<ir::Function>> m_functions;
    ir::Function* m_currentFunction;
    ir::BasicBlock* m_currentBlock;
    IrSymbolTable m_symbolTable;
    std::vector<std::pair<ir::BasicBlock*, ir::BasicBlock*>> m_breakableScopes; // (breakBlock, continueBlock)
    bool m_error = false;
};
