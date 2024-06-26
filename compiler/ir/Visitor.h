#pragma once

#include "Instructions.h"
#include "Terminators.h"
#include "BasicBlock.h"

namespace ir {
    class Visitor {
      public:
        virtual ~Visitor() = default;

        void visitBase(Visitable& visitable) {
            visitable.accept(*this);
        }

        virtual void visit(Function& function) {
            visit(*function.prologue());
            for (auto& block : function.blocks()) {
                visitBase(*block);
            }
            visit(*function.epilogue());
        }

        virtual void visit(BasicBlock& block) {
            for (auto& instruction : block.instructions()) {
                visitBase(*instruction);
            }

            if (block.terminator()) {
                visitBase(*block.terminator());
            }
        }

        // Instructions
        virtual void visit(BinaryOp&) {}
        virtual void visit(UnaryOp&) {}
        virtual void visit(Assignment&) {}
        virtual void visit(Call&) {}
        virtual void visit(Cast&) {}
        virtual void visit(PointerRead&) {}
        virtual void visit(PointerWrite&) {}
        virtual void visit(AddressOf&) {}

        //  Terminators
        virtual void visit(BasicJump&) {}
        virtual void visit(ConditionalJump&) {}
    };
}
