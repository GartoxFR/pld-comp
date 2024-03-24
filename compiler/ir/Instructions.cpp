#include "Instructions.h"
#include "Visitor.h"

using namespace ir;

void BinaryOp::accept(Visitor& visitor) { visitor.visit(*this); }

void UnaryOp::accept(Visitor& visitor) { visitor.visit(*this); }

void Assignment::accept(Visitor& visitor) { visitor.visit(*this); }

void Call::accept(Visitor& visitor) { visitor.visit(*this); }

void Cast::accept(Visitor& visitor) { visitor.visit(*this); }

void PointerRead::accept(Visitor& visitor) { visitor.visit(*this); }

void PointerWrite::accept(Visitor& visitor) { visitor.visit(*this); }

void AddressOf::accept(Visitor& visitor) { visitor.visit(*this); }
