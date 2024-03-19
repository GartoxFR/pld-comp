#include "./IrGenVisitor.h"
#include "generated/ifccParser.h"
#include "ir/Instructions.h"

using namespace ir;

const Function PUTCHAR("putchar", 1);
const Function GETCHAR("getchar", 0);

IrGenVisitor::IrGenVisitor() {
    m_symbolTable.declareFunction(PUTCHAR);
    m_symbolTable.declareFunction(GETCHAR);
}

std::any IrGenVisitor::visitFunction(ifccParser::FunctionContext *ctx)  {
    m_symbolTable.enterNewLocalScope();

    std::string functionName = ctx->IDENT()->getText();
    m_functions.push_back(std::make_unique<Function>(functionName, ctx->functionArg().size()));
    m_currentFunction = m_functions.back().get();

    m_symbolTable.declareFunction(*m_currentFunction);

    for (auto arg : ctx->functionArg()) {
        std::string argName = arg->IDENT()->getText();
        Local argLocal = m_currentFunction->newLocal(argName);
        m_symbolTable.declareLocalVariable(argName, argLocal);
    }

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

    m_currentBlock->terminate<BasicJump>(m_currentFunction->epilogue());

    BasicBlock* unreachableBlock = m_currentFunction->newBlock();
    unreachableBlock->terminate<BasicJump>(m_currentFunction->epilogue());

    m_currentBlock = unreachableBlock;
    

    return 0;
}
std::any IrGenVisitor::visitBreak(ifccParser::BreakContext *ctx) {
    if (m_breakableScopes.empty()) {
        error = true;
        std::cerr << "Error: Break not in a loop";
        return 0;
    }

    BasicBlock* endBlock = m_breakableScopes.back().first;
    m_currentBlock->terminate<BasicJump>(endBlock);

    BasicBlock* unreachableBlock = m_currentFunction->newBlock();
    unreachableBlock->terminate<BasicJump>(m_currentFunction->epilogue());

    m_currentBlock = unreachableBlock;

    return 0;
}

std::any IrGenVisitor::visitContinue(ifccParser::ContinueContext *ctx) {
    if (m_breakableScopes.empty()) {
        error = true;
        std::cerr << "Error: Continue not in a loop";
        return 0;
    }

    BasicBlock* testBlock = m_breakableScopes.back().second;
    m_currentBlock->terminate<BasicJump>(testBlock);

    BasicBlock* unreachableBlock = m_currentFunction->newBlock();
    unreachableBlock->terminate<BasicJump>(m_currentFunction->epilogue());

    m_currentBlock = unreachableBlock;

    return 0;
}

std::any IrGenVisitor::visitConst(ifccParser::ConstContext* ctx) {
    int value = std::stoi(ctx->CONST()->getText());
    Local res = m_currentFunction->newLocal();
    m_currentBlock->emit<Assignment>(res, Immediate(value));
    return res;
}

std::any IrGenVisitor::visitCharLiteral(ifccParser::CharLiteralContext *ctx) {
    auto text = ctx->CHAR()->getText();
    int value;
    if (text[1] != '\\') {
        value = ctx->CHAR()->getText()[1];
    } else {
        switch (text[2]) {
            case 'n': value = '\n'; break;
            case 'r': value = '\r'; break;
            case 't': value = '\t'; break;
            case '\'': value = '\''; break;
            case '\\': value = '\\'; break;
            case '0': value = '\0'; break;

            // Default value should never reached due to grammar
            default: value = 42; break;
        }
    }

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
    m_breakableScopes.push_back({endBlock, testBlock});
    visit(ctx->stmt());

    m_breakableScopes.pop_back();
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

    switch (ctx->PRODUCT_OP()->getText()[0]) {
        case '*': op = BinaryOpKind::MUL; break;
        case '/': op = BinaryOpKind::DIV; break;
        case '%': op = BinaryOpKind::MOD; break;
        default: return 0;
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

std::any IrGenVisitor::visitCall(ifccParser::CallContext *ctx) {
    if (!m_symbolTable.checkFunction(ctx->IDENT()->getText(), ctx->expr().size())) {
        return m_currentFunction->newLocal();
    }

    Local res = m_currentFunction->newLocal();
    std::vector<RValue> args;
    args.reserve(ctx->expr().size());
    for (auto& arg : ctx->expr()) {
        Local local = std::any_cast<Local>(visit(arg));
        args.push_back(local);
    }

    m_currentBlock->emit<Call>(res, ctx->IDENT()->getText(), args);

    return res;
}

std::any IrGenVisitor::visitLogicalOr(ifccParser::LogicalOrContext *ctx) {
    Local res = std::any_cast<Local>(visit(ctx->expr(0)));

    BasicBlock* orBlock = m_currentFunction->newBlock();
    BasicBlock* endBlock = m_currentFunction->newBlock();

    endBlock->terminate<ConditionalJump>(res, endBlock, orBlock);
    m_currentBlock->terminator().swap(endBlock->terminator());

    orBlock->terminate<BasicJump>(endBlock);

    m_currentBlock = orBlock;
    Local orRes = std::any_cast<Local>(visit(ctx->expr(1)));
    m_currentBlock->emit<Assignment>(res, orRes);

    m_currentBlock = endBlock;

    return res;
}

std::any IrGenVisitor::visitLogicalAnd(ifccParser::LogicalAndContext *ctx) {
    Local res = std::any_cast<Local>(visit(ctx->expr(0)));

    BasicBlock* andBlock = m_currentFunction->newBlock();
    BasicBlock* endBlock = m_currentFunction->newBlock();

    endBlock->terminate<ConditionalJump>(res, andBlock, endBlock);
    m_currentBlock->terminator().swap(endBlock->terminator());

    andBlock->terminate<BasicJump>(endBlock);

    m_currentBlock = andBlock;
    Local orRes = std::any_cast<Local>(visit(ctx->expr(1)));
    m_currentBlock->emit<Assignment>(res, orRes);

    m_currentBlock = endBlock;

    return res;
}
