#include "Function.h"
#include "Visitor.h"

using namespace ir;

void Function::accept(Visitor& visitor) {
    visitor.visit(*this);
}
