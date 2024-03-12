#include "Terminators.h"
#include "Visitor.h"

using namespace ir;

void BasicJump::accept(Visitor& visitor) { visitor.visit(*this); }

void ConditionalJump::accept(Visitor& visitor) { visitor.visit(*this); }
