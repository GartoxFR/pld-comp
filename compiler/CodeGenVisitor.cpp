#include "CodeGenVisitor.h"

antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext* ctx) {
    std::cout << ".globl main\n";
    std::cout << " main: \n";
    std::cout << "    pushq %rbp\n";
    std::cout << "    movq %rsp, %rbp\n";
    std::cout << "    subq  $" << globalSymbolTable.size() * 4 << ", %rsp\n";

    for (auto stmt : ctx->stmt())
        this->visit(stmt);
    this->visit(ctx->return_stmt());

    std::cout << "    movq %rbp, %rsp\n";
    std::cout << "    popq %rbp\n";
    std::cout << "    ret\n";

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext* ctx) {
    int x = 0;
    int retVar = std::any_cast<int>(visit(ctx->expr()));
    loadVariable(retVar);
    return 0;
}

std::any CodeGenVisitor::visitConst(ifccParser::ConstContext* ctx) {
    int res = nextTemp();
    int val = std::stoi(ctx->CONST()->getText());
    std::cout << "    movl $" << val << ", -" << res << "(%rbp)\n";

    return res;
}

std::any CodeGenVisitor::visitVar(ifccParser::VarContext* ctx) { return globalSymbolTable[ctx->IDENT()->getText()]; }

std::any CodeGenVisitor::visitInitializer(ifccParser::InitializerContext* ctx) {
    // No initializer -> nothing to do
    if (!ctx->expr()) {
        return 0;
    }

    auto ident = make_ident(ctx->IDENT());

    int res = std::any_cast<int>(visit(ctx->expr()));
    loadVariable(res);
    storeVariable(globalSymbolTable[ident]);

    return 0;
}

std::any CodeGenVisitor::visitAssign(ifccParser::AssignContext* ctx) {
    int temp = std::any_cast<int>(visit(ctx->expr()));
    loadVariable(temp);
    storeVariable(globalSymbolTable[ctx->IDENT()->getText()]);
    return 0;
}

void CodeGenVisitor::loadVariable(int temp) { std::cout << "    movl -" << temp << "(%rbp), %eax\n"; }

void CodeGenVisitor::storeVariable(int temp) { std::cout << "    movl %eax, -" << temp << "(%rbp)\n"; }

std::any CodeGenVisitor::visitSumOp(ifccParser::SumOpContext* ctx) {
    int res = nextTemp();
    int left = std::any_cast<int>(visit(ctx->expr(0)));
    int right = std::any_cast<int>(visit(ctx->expr(1)));

    loadVariable(left);

    if (ctx->SUM_OP()->getText() == "+") {
        std::cout << "    addl ";
    } else {
        std::cout << "    subl ";
    }

    std::cout << "-" << right << "(%rbp), %eax\n";

    storeVariable(res);

    return res;
}

std::any CodeGenVisitor::visitProductOp(ifccParser::ProductOpContext* ctx) {
    int res = nextTemp();
    int left = std::any_cast<int>(visit(ctx->expr(0)));
    int right = std::any_cast<int>(visit(ctx->expr(1)));

    loadVariable(left);

    std::cout << "    imull -" << right << "(%rbp), %eax\n";

    storeVariable(res);

    return res;
}

std::any CodeGenVisitor::visitUnarySumOp(ifccParser::UnarySumOpContext* ctx) {
    int operand = std::any_cast<int>(visit(ctx->expr()));
    // Nothing to do for unary +
    if (ctx->SUM_OP()->getText() == "+")
        return operand;

    int res = nextTemp();
    loadVariable(operand);
    std::cout << "    negl %eax\n";
    storeVariable(res);

    return res;
}
