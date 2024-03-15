#pragma once

#include "Visitable.h"
#include "Function.h"
#include "Instructions.h"
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

    class ConditionalJump : public Terminator {
      public:
        ConditionalJump(const RValue& condition, BasicBlock* m_trueTarget, BasicBlock* m_falseTarget) :
            m_condition(condition), m_trueTarget(m_trueTarget), m_falseTarget(m_falseTarget) {}

        void accept(Visitor& visitor) override;

        void print(std::ostream& out) const override {
            out << "cond_jump " << m_condition << " [true -> " << m_trueTarget->label() << ", false -> "
                << m_falseTarget->label() << "]";
        }

        BasicBlock* trueTarget() { return m_trueTarget; }

        BasicBlock* falseTarget() { return m_falseTarget; }

        const RValue& condition() const { return m_condition; }
        RValue& condition() { return m_condition; }

      private:
        RValue m_condition;
        BasicBlock* m_trueTarget;
        BasicBlock* m_falseTarget;
    };
}
