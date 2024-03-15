#include "Instructions.h"
#include "Visitor.h"

using namespace ir;

void BinaryOp::accept(Visitor& visitor) { visitor.visit(*this); }

void UnaryOp::accept(Visitor& visitor) { visitor.visit(*this); }

void Assignment::accept(Visitor& visitor) { visitor.visit(*this); }

void Call::accept(Visitor& visitor) { visitor.visit(*this); }
