#pragma once

#include "Visitable.h"
#include <cstdint>
#include <ostream>
#include <string>
#include <variant>

namespace ir {
    // Any linear Instruction in the IR (no jumps)
    class Instruction : public Visitable {
      public:
        virtual ~Instruction() = default;
        virtual void print(std::ostream& out) const = 0;
        virtual void accept(Visitor& visitor) override = 0;

        friend inline std::ostream& operator<<(std::ostream& out, const Instruction& self) {
            self.print(out);
            return out;
        }
    };

    // Overload the << operator to print any Instruction
    template <typename InstrT>
        requires std::derived_from<InstrT, Instruction> or std::same_as<InstrT, Instruction>
    inline std::ostream& operator<<(std::ostream& out, const InstrT& self) {
        self.print(out);
        return out;
    }

    // A variable declared in the code with a name
    class Variable {

      public:
        explicit Variable(const std::string& ident) : ident(ident) {}

        const std::string& getIdent() const { return ident; }

        friend inline std::ostream& operator<<(std::ostream& out, const Variable& self) {
            return out << self.getIdent();
        }

      private:
        const std::string& ident;
    };

    // A temporary expression created when linearizing expressions
    class Temporary {
      public:
        explicit Temporary(uint32_t id) : id(id) {}

        uint32_t getId() const { return id; }

        friend inline std::ostream& operator<<(std::ostream& out, const Temporary& self) {
            return out << "t" << self.getId();
        }

      private:
        uint32_t id;
    };

    // An immediate value (constant like 5 or 42)
    class Immediate {
      public:
        explicit Immediate(uint32_t value) : value(value) {}

        uint32_t getValue() const { return value; }

        friend inline std::ostream& operator<<(std::ostream& out, const Immediate& self) {
            return out << self.getValue();
        }

      private:
        uint32_t value;
    };

    // Anything that can be used as an operand
    using Operand = std::variant<Variable, Temporary, Immediate>;

    // Anything that can be assigned to
    using Place = std::variant<Variable, Temporary>;

    // Overload to be able to print Operands
    inline std::ostream& operator<<(std::ostream& out, const Operand& operand) {
        std::visit([&](auto& op) { out << op; }, operand);
        return out;
    }

    // Overload to be able to print Places
    inline std::ostream& operator<<(std::ostream& out, const Place& place) {
        std::visit([&](auto& op) { out << op; }, place);
        return out;
    }

    enum class BinaryOpKind {
        ADD,
        SUB,
        MUL,
        DIV,
    };

    inline std::ostream& operator<<(std::ostream& out, const BinaryOpKind& op) {
        switch (op) {
            case BinaryOpKind::ADD: return out << "+";
            case BinaryOpKind::SUB: return out << "-";
            case BinaryOpKind::MUL: return out << "*";
            case BinaryOpKind::DIV: return out << "/";
        }
        return out;
    }

    // Instruction of the form "destination := left operation right"
    // instruction is a Place
    // left and right are Operands
    class BinaryOp : public Instruction {
      public:
        BinaryOp(const Place& destination, const Operand& left, const Operand& right, BinaryOpKind operation) :
            Instruction(), destination(destination), left(left), right(right), operation(operation) {}

        void print(std::ostream& out) const override {
            out << destination << " := " << left << " " << operation << " " << right;
        }

        void accept(Visitor& visitor) override;

        const Place& getDestination() const { return destination; }
        const Operand& getLeft() const { return left; }
        const Operand& getRight() const { return right; }
        BinaryOpKind getOperation() const { return operation; }

      private:
        Place destination;
        Operand left;
        Operand right;
        BinaryOpKind operation;
    };

    // Instruction of the form "destination := source"
    class Copy : public Instruction {
      public:
        Copy(const Place& destination, const Operand& source) :
            Instruction(), destination(destination), source(source) {}

        void print(std::ostream& out) const override { out << destination << " := " << source; }

        const Place& getDestination() const { return destination; }

        const Operand& getSource() const { return source; }

        void accept(Visitor& visitor) override;

      private:
        Place destination;
        Operand source;
    };
}
