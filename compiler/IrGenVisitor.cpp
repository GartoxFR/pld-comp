#include "./IrGenVisitor.h"
#include "generated/ifccParser.h"
#include "ir/Instructions.h"

using namespace ir;
std::any IrGenVisitor::visitProg(ifccParser::ProgContext* ctx) {
    std::string functionName = "main";
    m_functions.push_back(std::make_unique<Function>(functionName, 0));
    m_currentFunction = m_functions.back().get();

    m_symbolTable.enterNewLocalScope();

    BasicBlock* prologue = m_currentFunction->prologue();
    BasicBlock* content = m_currentFunction->newBlock();
    BasicBlock* epilogue = m_currentFunction->epilogue();

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
    std::string ident = ctx->IDENT()->getText();

    // TODO: Check if variable exists and error otherwise

    return m_symbolTable.getLocalVariable(ident);
}

std::any IrGenVisitor::visitBlock(ifccParser::BlockContext *ctx) {
    m_symbolTable.enterNewLocalScope();
    visitChildren(ctx);
    m_symbolTable.exitLocalScope();
    
    return 0;
}

std::any IrGenVisitor::visitInitializer(ifccParser::InitializerContext* ctx) {

    std::string ident = ctx->IDENT()->getText();

    Local local = m_currentFunction->newLocal(ident);

    m_symbolTable.declareLocalVariable(ident, local);

    // Variable is initialized
    if (ctx->expr()) {
        Local res = std::any_cast<Local>(visit(ctx->expr()));
        m_currentBlock->emit<Assignment>(local, res);
    }
    return 0;
}
std::any IrGenVisitor::visitAssign_stmt(ifccParser::Assign_stmtContext* ctx) {
    std::string ident = ctx->IDENT()->getText();
    Local local = m_symbolTable.getLocalVariable(ident);
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

std::any IrGenVisitor::visitUnaryOp(ifccParser::ExprContext* operand, ir::UnaryOpKind op) {
    Local res = m_currentFunction->newLocal();
    Local operandRes = std::any_cast<Local>(visit(operand));
    m_currentBlock->emit<UnaryOp>(res, operandRes, op);
    return res;
}

std::any IrGenVisitor::visitUnarySumOp(ifccParser::UnarySumOpContext* ctx) {
    if (ctx->SUM_OP()->getText() == "-") {
        return visitUnaryOp(ctx->expr(), UnaryOpKind::MINUS);
    } else {
        // Nothing to do for unary "+"
        return visit(ctx->expr());
    }
}
