#include "BlockReordering.h"

using namespace ir;

void BlockReorderingVisitor::visit(Function& function) {
    m_currentFunction = &function;
    visitBase(*function.prologue()->terminator());
    while (!m_toVisit.empty()) {
        auto current = std::move(m_toVisit.back());
        m_toVisit.pop_back();

        if (current->terminator()) {
            visitBase(*current->terminator());
            m_finalBlocks.push_back(std::move(current));
        }
    }

    function.blocks().swap(m_finalBlocks);
}

void BlockReorderingVisitor::visit(BasicJump& jump) {
    visitNext(jump.target());
}

void BlockReorderingVisitor::visit(ConditionalJump& jump) {
    visitNext(jump.trueTarget());
    visitNext(jump.falseTarget());
}
