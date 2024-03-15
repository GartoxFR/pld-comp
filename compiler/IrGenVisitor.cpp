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

std::any IrGenVisitor::visitIf(ifccParser::IfContext *ctx) {
    Local cond = std::any_cast<Local>(visit(ctx->expr()));

    BasicBlock* thenBlock = m_currentFunction->newBlock();
    BasicBlock* elseBlock = m_currentFunction->newBlock();
    BasicBlock* endBlock = m_currentFunction->newBlock();

    endBlock->terminate<ConditionalJump>(cond, thenBlock, elseBlock);
    m_currentBlock->terminator().swap(endBlock->terminator());

    thenBlock->terminate<BasicJump>(endBlock);
    elseBlock->terminate<BasicJump>(endBlock);

    m_currentBlock = thenBlock;
    visit(ctx->then);

    if (ctx->else_) {
        m_currentBlock = elseBlock;
        visit(ctx->else_);
    }

    m_currentBlock = endBlock;
    
    return 0;
}

std::any IrGenVisitor::visitWhile(ifccParser::WhileContext *ctx) {
    // Setup the test block
    BasicBlock* testBlock = m_currentFunction->newBlock();
    testBlock->terminate<BasicJump>(testBlock);
    m_currentBlock->terminator().swap(testBlock->terminator());

    m_currentBlock = testBlock;
    Local testRes = std::any_cast<Local>(visit(ctx->expr()));

    BasicBlock* bodyBlock = m_currentFunction->newBlock();
    BasicBlock* endBlock = m_currentFunction->newBlock();

    endBlock->terminate<ConditionalJump>(testRes, bodyBlock, endBlock);
    m_currentBlock->terminator().swap(endBlock->terminator());
    bodyBlock->terminate<BasicJump>(testBlock);

    m_currentBlock = bodyBlock;
    visit(ctx->stmt());

    m_currentBlock = endBlock;
    
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
std::any IrGenVisitor::visitAssign(ifccParser::AssignContext* ctx) {
    std::string ident = ctx->IDENT()->getText();
    Local local = m_symbolTable.getLocalVariable(ident);
    Local res = std::any_cast<Local>(visit(ctx->expr()));
    m_currentBlock->emit<Assignment>(local, res);
    return local;
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

std::any IrGenVisitor::visitCmpOp(ifccParser::CmpOpContext* ctx) {
    BinaryOpKind op;
    std::string opStr = ctx->CMP_OP()->getText();

    if (opStr == ">=") {
        op = BinaryOpKind::CMP_GE;
    } else if (opStr == "<=") {
        op = BinaryOpKind::CMP_LE;
    } else if (opStr == ">") {
        op = BinaryOpKind::CMP_G;
    } else {
        op = BinaryOpKind::CMP_L;
    }

    return visitBinaryOp(ctx->expr(0), ctx->expr(1), op);
}

std::any IrGenVisitor::visitEqOp(ifccParser::EqOpContext* ctx) {
    BinaryOpKind op;
    if (ctx->EQ_OP()->getText() == "==") {
        op = BinaryOpKind::EQ;
    } else {
        op = BinaryOpKind::NEQ;
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

std::any IrGenVisitor::visitUnaryOp(ifccParser::UnaryOpContext* ctx) {
    UnaryOpKind kind;
    switch (ctx->UNARY_OP()->getText()[0]) {
        case '!': kind = UnaryOpKind::NOT; break;
        default: return visit(ctx->expr());
    }
    return visitUnaryOp(ctx->expr(), kind);
}

std::any IrGenVisitor::visitUnarySumOp(ifccParser::UnarySumOpContext* ctx) {
    UnaryOpKind kind;
    switch (ctx->SUM_OP()->getText()[0]) {
        case '-': kind = UnaryOpKind::MINUS; break;
        default: return visit(ctx->expr());
    }
    return visitUnaryOp(ctx->expr(), kind);
}
