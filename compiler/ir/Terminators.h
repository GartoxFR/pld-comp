#pragma once

#include "Visitable.h"
#include "ControlFlowGraph.h"
#include <ostream>

namespace ir {

    // Always represent the last instruction of a block to link with the next one
    class Terminator : public Visitable {
      public:
        virtual ~Terminator() = default;
        virtual void print(std::ostream& out) const = 0;
        virtual void accept(Visitor& visitor) override = 0;

        friend inline std::ostream& operator<<(std::ostream& out, const Terminator& self) {
            self.print(out);
            return out;
        }
    };

    // Overload the << operator to print any Instruction
    template <typename TermT>
        requires std::derived_from<TermT, Terminator> or std::same_as<TermT, Terminator>
    inline std::ostream& operator<<(std::ostream& out, const TermT& self) {
        self.print(out);
        return out;
    }

    class BasicJump : public Terminator {
        void accept(Visitor& visitor) override;

        void print(std::ostream& out) const override {
            out << "jump";
        }

      private:
        BasicBlock* target;
    };

}
