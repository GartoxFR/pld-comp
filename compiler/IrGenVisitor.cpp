#include "./IrGenVisitor.h"
#include "generated/ifccParser.h"
#include "ir/Instructions.h"
#include "Type.h"
#include <memory>

using namespace ir;

static std::unique_ptr<Function> PUTCHAR;
static std::unique_ptr<Function> GETCHAR;
static std::unique_ptr<Function> MALLOC;
static std::unique_ptr<Function> FREE;
static std::unique_ptr<Function> PRINTF;

struct LValueResult {
    ir::Local local;
    bool address; // Wether local should be consider as an address or not
};

IrGenVisitor::IrGenVisitor() {
    PUTCHAR = std::unique_ptr<Function>(new Function("putchar", {types::INT}, types::INT));
    GETCHAR = std::unique_ptr<Function>(new Function("getchar", {}, types::INT));
    MALLOC = std::unique_ptr<Function>(new Function("malloc", {types::LONG}, make_pointer_type(types::VOID)));
    FREE = std::unique_ptr<Function>(new Function("free", {make_pointer_type(types::VOID)}, types::VOID));
    PRINTF = std::unique_ptr<Function>(new Function("printf", {make_pointer_type(types::CHAR)}, types::INT, true));

    m_symbolTable.declareFunction(*PUTCHAR);
    m_symbolTable.declareFunction(*GETCHAR);
    m_symbolTable.declareFunction(*MALLOC);
    m_symbolTable.declareFunction(*FREE);
    m_symbolTable.declareFunction(*PRINTF);
}

std::any IrGenVisitor::visitFunction(ifccParser::FunctionContext* ctx) {
    m_symbolTable.enterNewLocalScope();

    std::string functionName = ctx->IDENT()->getText();
    auto returnType = std::any_cast<const Type*>(visit(ctx->type()));
    m_functions.push_back(std::make_unique<Function>(functionName, ctx->functionArg().size(), returnType));
    m_currentFunction = m_functions.back().get();

    m_symbolTable.declareFunction(*m_currentFunction);

    for (auto arg : ctx->functionArg()) {
        auto type = std::any_cast<const Type*>(visit(arg->type()));
        std::string argName = arg->IDENT()->getText();
        Local argLocal = m_currentFunction->newLocal(argName, type);
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
    if (ctx->expr()) {
        Local res = std::any_cast<Local>(visit(ctx->expr()));
        Local returnLocal = m_currentFunction->returnLocal();

        if (res.type() != returnLocal.type()) {
            res = emitCast(res, returnLocal.type());
        }

        m_currentBlock->emit<Assignment>(returnLocal, res);
    }

    m_currentBlock->terminate<BasicJump>(m_currentFunction->epilogue());

    BasicBlock* unreachableBlock = m_currentFunction->newBlock();
    unreachableBlock->terminate<BasicJump>(m_currentFunction->epilogue());

    m_currentBlock = unreachableBlock;

    return 0;
}
std::any IrGenVisitor::visitBreak(ifccParser::BreakContext* ctx) {
    if (m_breakableScopes.empty()) {
        m_error = true;
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

std::any IrGenVisitor::visitContinue(ifccParser::ContinueContext* ctx) {
    if (m_breakableScopes.empty()) {
        m_error = true;
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
    Local res = m_currentFunction->newLocal(types::INT);
    m_currentBlock->emit<Assignment>(res, Immediate(value, types::INT));
    return res;
}

std::any IrGenVisitor::visitCharLiteral(ifccParser::CharLiteralContext* ctx) {
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

    Local res = m_currentFunction->newLocal(types::CHAR);
    m_currentBlock->emit<Assignment>(res, Immediate(value, types::CHAR));
    return res;
}
std::any IrGenVisitor::visitStringLiteral(ifccParser::StringLiteralContext *ctx) {
    std::string str = ctx->STRING()->getText();
    str = str.substr(1, str.size() - 2);

    StringLiteral literal = m_currentFunction->newLiteral(std::move(str));
    Local res = m_currentFunction->newLocal(make_pointer_type(types::CHAR));
    m_currentBlock->emit<AddressOf>(res, literal);
    return res;
}

std::any IrGenVisitor::visitLvalueExpr(ifccParser::LvalueExprContext* ctx) {
    LValueResult lvalue = std::any_cast<LValueResult>(visit(ctx->lvalue()));
    if (!lvalue.address) {
        return lvalue.local;
    } else {
        Local res = m_currentFunction->newLocal(lvalue.local.type()->target());
        m_currentBlock->emit<PointerRead>(res, lvalue.local);
        return res;
    }
}

std::any IrGenVisitor::visitBlock(ifccParser::BlockContext* ctx) {
    m_symbolTable.enterNewLocalScope();
    visitChildren(ctx);
    m_symbolTable.exitLocalScope();

    return 0;
}

std::any IrGenVisitor::visitIf(ifccParser::IfContext* ctx) {
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

std::any IrGenVisitor::visitWhile(ifccParser::WhileContext* ctx) {
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

std::any IrGenVisitor::visitDeclare_stmt(ifccParser::Declare_stmtContext* ctx) {
    auto type = std::any_cast<const Type*>(visit(ctx->type()));

    for (auto initializerCtx : ctx->initializer()) {

        std::string ident = initializerCtx->IDENT()->getText();
        Local local = m_currentFunction->newLocal(ident, type);

        m_symbolTable.declareLocalVariable(ident, local);

        // Variable is initialized
        if (initializerCtx->expr()) {
            Local res = std::any_cast<Local>(visit(initializerCtx->expr()));
            if (res.type() != type) {
                res = emitCast(res, type);
            }
            m_currentBlock->emit<Assignment>(local, res);
        }
    }
    return 0;
}

std::any IrGenVisitor::visitAssign(ifccParser::AssignContext* ctx) {
    LValueResult lvalue = std::any_cast<LValueResult>(visit(ctx->lvalue()));
    Local res = std::any_cast<Local>(visit(ctx->expr()));

    if (ctx->ASSIGN_OP()) {
        BinaryOpKind op;
        std::string opStr = ctx->ASSIGN_OP()->getText();
        if (opStr == "+=") {
            op = BinaryOpKind::ADD;
        } else if (opStr == "-=") {
            op = BinaryOpKind::SUB;
        } else if (opStr == "*=") {
            op = BinaryOpKind::MUL;
        } else if (opStr == "/=") {
            op = BinaryOpKind::DIV;
        } else if (opStr == "%=") {
            op = BinaryOpKind::MOD;
        } else if (opStr == "&=") {
            op = BinaryOpKind::BIT_AND;
        } else if (opStr == "^=") {
            op = BinaryOpKind::BIT_XOR;
        } else if (opStr == "|=") {
            op = BinaryOpKind::BIT_OR;
        } else {
            op = BinaryOpKind::BIT_OR;
        }

        if (!lvalue.address) {
            auto local = lvalue.local;
            if (res.type() != local.type()) {
                // TODO: Check if this is right semantic
                res = emitCast(res, local.type());
            }

            if (lvalue.local.type()->isPtr()) {
                Local result = visitPointerBinaryOp(local, res, op);
                m_currentBlock->emit<Assignment>(local, result);
            } else {
                m_currentBlock->emit<BinaryOp>(local, local, res, op);
            }

            return local;
        } else {
            auto local = m_currentFunction->newLocal(lvalue.local.type()->target());
            m_currentBlock->emit<PointerRead>(local, lvalue.local);
            if (local.type()->isPtr()) {
                Local result = visitPointerBinaryOp(local, res, op);
                m_currentBlock->emit<Assignment>(local, result);
            } else {
                m_currentBlock->emit<BinaryOp>(local, local, res, op);
            }
            m_currentBlock->emit<PointerWrite>(lvalue.local, local);

            return local;
        }

    } else {
        if (!lvalue.address) {
            auto local = lvalue.local;
            if (res.type() != local.type()) {
                res = emitCast(res, local.type());
            }
            m_currentBlock->emit<Assignment>(local, res);
        } else {
            auto type = lvalue.local.type()->target();
            if (res.type() != type) {
                res = emitCast(res, type);
            }

            m_currentBlock->emit<PointerWrite>(lvalue.local, res);
        }
        return res;
    }
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

    if (ctx->PRODUCT_OP()) {
        switch (ctx->PRODUCT_OP()->getText()[0]) {
            case '/': op = BinaryOpKind::DIV; break;
            case '%': op = BinaryOpKind::MOD; break;
            default: return 0;
        }
    } else {
        op = BinaryOpKind::MUL;
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

    return visitBinaryOp(ctx->expr(0), ctx->expr(1), op, true);
}

std::any IrGenVisitor::visitEqOp(ifccParser::EqOpContext* ctx) {
    BinaryOpKind op;
    if (ctx->EQ_OP()->getText() == "==") {
        op = BinaryOpKind::EQ;
    } else {
        op = BinaryOpKind::NEQ;
    }

    return visitBinaryOp(ctx->expr(0), ctx->expr(1), op, true);
}

ir::Local IrGenVisitor::visitPointerBinaryOp(ir::Local left, ir::Local right, ir::BinaryOpKind op) {
    if (left.type() != right.type()) {
        right = emitCast(right, left.type());
    }

    Local offset = m_currentFunction->newLocal(left.type());

    size_t size = left.type()->target()->size();
    // void* special case
    if (size == 0) {
        size = 1;
    }

    m_currentBlock->emit<BinaryOp>(offset, right, Immediate(size, left.type()), BinaryOpKind::MUL);

    Local res = m_currentFunction->newLocal(left.type());
    m_currentBlock->emit<BinaryOp>(res, left, offset, BinaryOpKind::ADD);

    return res;
}

std::any IrGenVisitor::visitBinaryOp(
    ifccParser::ExprContext* left, ifccParser::ExprContext* right, ir::BinaryOpKind op, bool comp
) {
    Local leftRes = std::any_cast<Local>(visit(left));
    Local rightRes = std::any_cast<Local>(visit(right));

    if (leftRes.type()->isPtr() && (op == BinaryOpKind::ADD || op == BinaryOpKind::SUB)) {
        return visitPointerBinaryOp(leftRes, rightRes, op);
    }

    if (rightRes.type()->isPtr() && op == BinaryOpKind::ADD) {
        return visitPointerBinaryOp(rightRes, leftRes, op);
    }

    if (leftRes.type()->isPtr() || rightRes.type()->isPtr()) {
        m_error = true;
        std::cerr << "Invalid operand types '" << leftRes.type()->name() << "' and '" << rightRes.type()->name()
                  << "' for operator " << op << std::endl;

        return m_currentFunction->invalidLocal();
    }

    const Type* resType;
    if (leftRes.type() == rightRes.type()) {
        resType = leftRes.type();
    } else if (leftRes.type()->size() < rightRes.type()->size()) {
        resType = rightRes.type();
        leftRes = emitCast(leftRes, resType);
    } else if (leftRes.type()->size() >= rightRes.type()->size()) {
        resType = leftRes.type();
        rightRes = emitCast(rightRes, resType);
    } else {
    }

    if (comp) {
        resType = types::BOOL;
    }

    Local res = m_currentFunction->newLocal(resType);
    m_currentBlock->emit<BinaryOp>(res, leftRes, rightRes, op);
    return res;
}

std::any IrGenVisitor::visitUnaryOp(ifccParser::ExprContext* operand, ir::UnaryOpKind op, bool comp) {
    Local operandRes = std::any_cast<Local>(visit(operand));
    auto resType = operandRes.type();
    if (comp) {
        resType = types::BOOL;
    }
    Local res = m_currentFunction->newLocal(resType);
    m_currentBlock->emit<UnaryOp>(res, operandRes, op);
    return res;
}

std::any IrGenVisitor::visitUnaryOp(ifccParser::UnaryOpContext* ctx) {
    UnaryOpKind kind;
    switch (ctx->UNARY_OP()->getText()[0]) {
        case '!': kind = UnaryOpKind::NOT; break;
        default: return visit(ctx->expr());
    }
    return visitUnaryOp(ctx->expr(), kind, true);
}

std::any IrGenVisitor::visitUnarySumOp(ifccParser::UnarySumOpContext* ctx) {
    UnaryOpKind kind;
    switch (ctx->SUM_OP()->getText()[0]) {
        case '-': kind = UnaryOpKind::MINUS; break;
        default: return visit(ctx->expr());
    }
    return visitUnaryOp(ctx->expr(), kind);
}

std::any IrGenVisitor::visitCall(ifccParser::CallContext* ctx) {
    auto ident = ctx->IDENT()->getText();
    if (!m_symbolTable.checkFunction(ident, ctx->expr().size())) {
        return m_currentFunction->newLocal(types::VOID);
    }

    const Function* function = m_symbolTable.getFunction(ident);
    Local res = m_currentFunction->newLocal(function->returnLocal().type());
    std::vector<RValue> args;
    args.reserve(ctx->expr().size());
    size_t i = 1;
    for (auto& arg : ctx->expr()) {
        Local local = std::any_cast<Local>(visit(arg));
        if (i <= function->argCount()) {
            auto argType = function->locals()[i].type();
            if (local.type() != argType) {
                local = emitCast(local, argType);
            }
        }
        args.push_back(local);
        ++i;
    }

    m_currentBlock->emit<Call>(res, ident, args, function->variadic());

    return res;
}

std::any IrGenVisitor::visitBitAnd(ifccParser::BitAndContext* ctx) {
    BinaryOpKind op = BinaryOpKind::BIT_AND;

    return visitBinaryOp(ctx->expr(0), ctx->expr(1), op);
}

std::any IrGenVisitor::visitBitXor(ifccParser::BitXorContext* ctx) {
    BinaryOpKind op = BinaryOpKind::BIT_XOR;

    return visitBinaryOp(ctx->expr(0), ctx->expr(1), op);
}

std::any IrGenVisitor::visitBitOr(ifccParser::BitOrContext* ctx) {
    BinaryOpKind op = BinaryOpKind::BIT_OR;

    return visitBinaryOp(ctx->expr(0), ctx->expr(1), op);
}

std::any IrGenVisitor::visitPreIncrDecrOp(ifccParser::PreIncrDecrOpContext* ctx) {
    BinaryOpKind op;
    std::string opStr = ctx->INCRDECR_OP()->getText();

    if (opStr == "++") {
        op = BinaryOpKind::ADD;
    } else {
        op = BinaryOpKind::SUB;
    }

    LValueResult res = std::any_cast<LValueResult>(visit(ctx->lvalue()));
    if (!res.address) {

        if (res.local.type()->isPtr()) {
            Local one = m_currentFunction->newLocal(res.local.type());
            m_currentBlock->emit<Assignment>(one, Immediate(1, res.local.type()));
            Local addResult = visitPointerBinaryOp(res.local, one, op);
            m_currentBlock->emit<Assignment>(res.local, addResult);
        } else {
            m_currentBlock->emit<BinaryOp>(res.local, res.local, Immediate(1, res.local.type()), op);
        }

        return res.local;
    } else {
        Local result = m_currentFunction->newLocal(res.local.type()->target());
        m_currentBlock->emit<PointerRead>(result, res.local);

        if (result.type()->isPtr()) {
            Local one = m_currentFunction->newLocal(res.local.type());
            m_currentBlock->emit<Assignment>(one, Immediate(1, res.local.type()));
            Local addResult = visitPointerBinaryOp(result, one, op);
            m_currentBlock->emit<PointerWrite>(res.local, result);
            m_currentBlock->emit<Assignment>(result, addResult);

        } else {
            m_currentBlock->emit<BinaryOp>(result, result, Immediate(1, result.type()), op);
        }

        m_currentBlock->emit<PointerWrite>(res.local, result);

        return result;
    }
    return res;
}

std::any IrGenVisitor::visitPostIncrDecrOp(ifccParser::PostIncrDecrOpContext* ctx) {
    BinaryOpKind op;
    std::string opStr = ctx->INCRDECR_OP()->getText();

    if (opStr == "++") {
        op = BinaryOpKind::ADD;
    } else {
        op = BinaryOpKind::SUB;
    }

    LValueResult res = std::any_cast<LValueResult>(visit(ctx->postLvalue()));
    if (!res.address) {
        Local temp = m_currentFunction->newLocal(res.local.type());
        m_currentBlock->emit<Assignment>(temp, res.local);
        if (res.local.type()->isPtr()) {
            Local one = m_currentFunction->newLocal(res.local.type());
            m_currentBlock->emit<Assignment>(one, Immediate(1, res.local.type()));
            Local addResult = visitPointerBinaryOp(res.local, one, op);
            m_currentBlock->emit<Assignment>(res.local, addResult);
        } else {
            m_currentBlock->emit<BinaryOp>(res.local, res.local, Immediate(1, res.local.type()), op);
        }

        return temp;
    } else {
        Local temp = m_currentFunction->newLocal(res.local.type()->target());
        m_currentBlock->emit<PointerRead>(temp, res.local);
        if (temp.type()->isPtr()) {
            Local one = m_currentFunction->newLocal(res.local.type());
            m_currentBlock->emit<Assignment>(one, Immediate(1, res.local.type()));
            Local result = visitPointerBinaryOp(temp, one, op);
            m_currentBlock->emit<PointerWrite>(res.local, result);
        } else {
            Local result = m_currentFunction->newLocal(temp.type());
            m_currentBlock->emit<BinaryOp>(result, temp, Immediate(1, result.type()), op);
            m_currentBlock->emit<PointerWrite>(res.local, result);
        }

        return temp;
    }
    
}

std::any IrGenVisitor::visitLogicalOr(ifccParser::LogicalOrContext* ctx) {
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

std::any IrGenVisitor::visitLogicalAnd(ifccParser::LogicalAndContext* ctx) {
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

std::any IrGenVisitor::visitSimpleType(ifccParser::SimpleTypeContext* ctx) {
    return make_simple_type(ctx->FLAT_TYPE()->getText());
}

std::any IrGenVisitor::visitPointerType(ifccParser::PointerTypeContext* ctx) {
    const Type* pointee = std::any_cast<const Type*>(visit(ctx->type()));
    return make_pointer_type(pointee);
}

ir::Local IrGenVisitor::emitCast(ir::Local source, const Type* targetType) {
    Local res = m_currentFunction->newLocal(targetType);
    m_currentBlock->emit<Cast>(res, source);
    return res;
}

std::any IrGenVisitor::visitLvalueVar(ifccParser::LvalueVarContext* ctx) {
    std::string ident = ctx->IDENT()->getText();

    // TODO: Check if variable exists and error otherwise

    return LValueResult{m_symbolTable.getLocalVariable(ident), false};
}
std::any IrGenVisitor::visitLvalueDeref(ifccParser::LvalueDerefContext* ctx) {
    auto res = std::any_cast<Local>(visit(ctx->expr()));
    if (!res.type()->isPtr()) {
        m_error = true;
        std::cerr << "Pointer dereference could not be applied on type " << res.type()->name() << std::endl;
        return LValueResult{m_currentFunction->invalidLocal(), true};
    }

    return LValueResult{res, true};
}
std::any IrGenVisitor::visitAddressOf(ifccParser::AddressOfContext* ctx) {
    auto lvalue = std::any_cast<LValueResult>(visit(ctx->lvalue()));

    if (lvalue.address) {
        return lvalue.local;
    } else {
        Local res = m_currentFunction->newLocal(make_pointer_type(lvalue.local.type()));
        m_currentBlock->emit<AddressOf>(res, lvalue.local);
        return res;
    }
}

std::any IrGenVisitor::visitLvalueIndex(ifccParser::LvalueIndexContext* ctx) {
    auto local = m_symbolTable.getLocalVariable(ctx->IDENT()->getText());

    if (!local.type()->isPtr()) {
        m_error = true;
        std::cerr << "Could not index type " << local.type()->name() << std::endl;
        return LValueResult{m_currentFunction->invalidLocal(), true};
    }

    auto right = std::any_cast<Local>(visit(ctx->expr()));

    return LValueResult{std::any_cast<Local>(visitPointerBinaryOp(local, right, BinaryOpKind::ADD)), true};
}

std::any IrGenVisitor::visitLvalueIndexPar(ifccParser::LvalueIndexParContext *ctx) {
    auto local = std::any_cast<Local>(visit(ctx->expr(0)));

    if (!local.type()->isPtr()) {
        m_error = true;
        std::cerr << "Could not index type " << local.type()->name() << std::endl;
        return LValueResult{m_currentFunction->invalidLocal(), true};
    }

    auto right = std::any_cast<Local>(visit(ctx->expr(1)));

    return LValueResult{std::any_cast<Local>(visitPointerBinaryOp(local, right, BinaryOpKind::ADD)), true};
}
