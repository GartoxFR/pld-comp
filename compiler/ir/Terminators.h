#pragma once

#include "Visitable.h"
#include "Function.h"
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
      public:
        explicit BasicJump(BasicBlock* target) : m_target(target) {}

        void accept(Visitor& visitor) override;

        void print(std::ostream& out) const override { out << "jump " << m_target->label(); }

        BasicBlock* target() { return m_target; }

      private:
        BasicBlock* m_target;
    };
}
