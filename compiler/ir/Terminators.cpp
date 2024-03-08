#include "Terminators.h"
#include "Visitor.h"

using namespace ir;

void Terminator::accept(Visitor& visitor) {
    visitor.visit(*this);
}
