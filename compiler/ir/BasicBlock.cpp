#include "BasicBlock.h"
#include "Visitor.h"

using namespace ir;

void BasicBlock::accept(Visitor& visitor) { 
    visitor.visit(*this); 
}

