#include "./IrGenVisitor.h"
#include "generated/ifccParser.h"
#include "ir/Instructions.h"

using namespace ir;
std::any IrGenVisitor::visitProg(ifccParser::ProgContext* ctx) {
    Ident functionName = make_ident("main");
    m_functions.push_back(std::make_unique<Function>(functionName, 0));
    m_currentFunction = m_functions.back().get();

    BasicBlock* prologue = m_currentFunction->newBlock();
    BasicBlock* content = m_currentFunction->newBlock();
    BasicBlock* epilogue = m_currentFunction->newBlock();

    prologue->terminate<BasicJump>(content);
    content->terminate<BasicJump>(epilogue);

    m_currentBlock = content;

    visitChildren(ctx);

    return 0;
}

std::any IrGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext* ctx) {
    Local res = std::any_cast<Local>(visit(ctx->expr()));
    Local returnLocal = m_currentFunction->returnLocal();

    m_currentBlock->emit<Assignment>(returnLocal, res);

    return 0;
}

std::any IrGenVisitor::visitConst(ifccParser::ConstContext* ctx) {
    int value = std::stoi(ctx->CONST()->getText());
    Local res = m_currentFunction->newLocal();
    m_currentBlock->emit<Assignment>(res, Immediate(value));
    return res;
}

std::any IrGenVisitor::visitVar(ifccParser::VarContext* ctx) {
    Ident ident = make_ident(ctx->IDENT());

    // TODO: Check if variable exists and error otherwise

    return m_localTable.at(ident);
}

std::any IrGenVisitor::visitInitializer(ifccParser::InitializerContext* ctx) {

    Ident ident = make_ident(ctx->IDENT());

    if (m_localTable.contains(ident)) {
        // TODO: Error
        return 0;
    }
    Local local = m_currentFunction->newLocal(ident);
    m_localTable.insert({ident, local});

    // Variable is initialized
    if (ctx->expr()) {
        Local res = std::any_cast<Local>(visit(ctx->expr()));
        m_currentBlock->emit<Assignment>(local, res);
    }
    return 0;
}
std::any IrGenVisitor::visitAssign_stmt(ifccParser::Assign_stmtContext* ctx) {
    Ident ident = make_ident(ctx->IDENT());
    Local local = m_localTable.at(ident);
    Local res = std::any_cast<Local>(visit(ctx->expr()));
    m_currentBlock->emit<Assignment>(local, res);
    return 0;
}

std::any IrGenVisitor::visitSumOp(ifccParser::SumOpContext* ctx) {
    BinaryOpKind op;
    if (ctx->SUM_OP()->getText() == "+") {
        op = BinaryOpKind::ADD;
    } else {
        op = BinaryOpKind::SUB;
    }

    return visitBinaryOp(ctx->expr(0), ctx->expr(1), op);
}
std::any IrGenVisitor::visitProductOp(ifccParser::ProductOpContext* ctx) {
    BinaryOpKind op;
    if (ctx->PRODUCT_OP()->getText() == "*") {
        op = BinaryOpKind::MUL;
    } else {
        op = BinaryOpKind::DIV;
    }

    return visitBinaryOp(ctx->expr(0), ctx->expr(1), op);
}

std::any
IrGenVisitor::visitBinaryOp(ifccParser::ExprContext* left, ifccParser::ExprContext* right, ir::BinaryOpKind op) {
    Local res = m_currentFunction->newLocal();
    Local leftRes = std::any_cast<Local>(visit(left));
    Local rightRes = std::any_cast<Local>(visit(right));
    m_currentBlock->emit<BinaryOp>(res, leftRes, rightRes, op);
    return res;
}

std::any IrGenVisitor::visitUnarySumOp(ifccParser::UnarySumOpContext* ctx) {
    // TODO: UnaryOP
    return 0;
}
