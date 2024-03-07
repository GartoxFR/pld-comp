#include "Instructions.h"
#include "Visitor.h"

using namespace ir;

void BinaryOp::accept(Visitor& visitor) {
    visitor.visit(*this);
}

void Copy::accept(Visitor& visitor) {
    visitor.visit(*this);
}
