#pragma once

#include "Visitable.h"
#include <cstdint>
#include <ostream>
#include <string>
#include <variant>

namespace ir {
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

    template <typename InstrT>
        requires std::derived_from<InstrT, Instruction>
    inline std::ostream& operator<<(std::ostream& out, const Instruction& self) {
        self.print(out);
        return out;
    }

    class Variable {

      public:
        explicit Variable(const std::string& ident) : ident(ident) {}

        const std::string& getIdent() const {
            return ident;
        }

        friend inline std::ostream& operator<<(std::ostream& out, const Variable& self) {
            return out << self.getIdent();
        }

      private:
        const std::string& ident;
    };

    class Temporary {
      public:
        explicit Temporary(uint32_t id) : id(id) {}

        uint32_t getId() const {
            return id;
        }

        friend inline std::ostream& operator<<(std::ostream& out, const Temporary& self) {
            return out << "t" << self.getId();
        }

      private:
        uint32_t id;
    };

    class Immediate {
      public:
        explicit Immediate(uint32_t value) : value(value) {}

        uint32_t getValue() const {
            return value;
        }

        friend inline std::ostream& operator<<(std::ostream& out, const Immediate& self) {
            return out << self.getValue();
        }

      private:
        uint32_t value;
    };

    using Operand = std::variant<Variable, Temporary, Immediate>;
    using Place = std::variant<Variable, Temporary>;

    inline std::ostream& operator<<(std::ostream& out, const Operand& operand) {
        std::visit([&](auto& op) { out << op; }, operand);
        return out;
    }

    inline std::ostream& operator<<(std::ostream& out, const Place& operand) {
        std::visit([&](auto& op) { out << op; }, operand);
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

    class BinaryOp : public Instruction {
      public:
        BinaryOp(const Place& destination, const Operand& left, const Operand& right, BinaryOpKind operation) :
            Instruction(), destination(destination), left(left), right(right), operation(operation) {}

        void print(std::ostream& out) const override {
            out << destination << " := " << left << " " << operation << " " << right;
        }

        void accept(Visitor& visitor) override;

        const Place& getDestination() const {
            return destination;
        }
        const Operand& getLeft() const {
            return left;
        }
        const Operand& getRight() const {
            return right;
        }
        BinaryOpKind getOperation() const {
            return operation;
        }

      private:
        Place destination;
        Operand left;
        Operand right;
        BinaryOpKind operation;
    };

    class Copy : public Instruction {
      public:
        Copy(const Place& destination, const Operand& source) :
            Instruction(), destination(destination), source(source) {}

        void print(std::ostream& out) const override {
            out << destination << " := " << source;
        }

        void accept(Visitor& visitor) override;

      private:
        Place destination;
        Operand source;
    };
}
