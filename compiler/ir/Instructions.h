#pragma once

#include "Visitable.h"
#include <cstdint>
#include <ostream>
#include <string>
#include <variant>
#include <unordered_map>
#include <vector>

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

    // A local variable being a temporary or named variable
    class Local {
      public:
        explicit Local(uint32_t id) : m_id(id) {}

        uint32_t id() const { return m_id; }

        friend inline std::ostream& operator<<(std::ostream& out, const Local& self) { return out << "_" << self.id(); }

      private:
        uint32_t m_id;
    };

    // An immediate value (constant like 5 or 42)
    class Immediate {
      public:
        explicit Immediate(uint32_t value) : m_value(value) {}

        int32_t value() const { return m_value; }

        friend inline std::ostream& operator<<(std::ostream& out, const Immediate& self) { return out << self.value(); }

      private:
        int32_t m_value;
    };

    // Anything that can be used as an operand
    using RValue = std::variant<Local, Immediate>;

    // Overload to be able to print RValues
    inline std::ostream& operator<<(std::ostream& out, const RValue& rvalue) {
        std::visit([&](auto& op) { out << op; }, rvalue);
        return out;
    }

    enum class BinaryOpKind {
        ADD,
        SUB,
        MUL,
        DIV,
        EQ,
        NEQ,
        CMP_L,
        CMP_G,
        CMP_LE,
        CMP_GE,
    };

    inline std::ostream& operator<<(std::ostream& out, const BinaryOpKind& op) {
        switch (op) {
            case BinaryOpKind::ADD: return out << "+";
            case BinaryOpKind::SUB: return out << "-";
            case BinaryOpKind::MUL: return out << "*";
            case BinaryOpKind::DIV: return out << "/";
            case BinaryOpKind::EQ: return out << "==";
            case BinaryOpKind::NEQ: return out << "!=";
            case BinaryOpKind::CMP_L: return out << "<";
            case BinaryOpKind::CMP_G: return out << ">";
            case BinaryOpKind::CMP_LE: return out << "<=";
            case BinaryOpKind::CMP_GE: return out << ">=";
        }
        return out;
    }

    // Instruction of the form "destination := left operation right"
    // instruction is a Local
    // left and right are RValues
    class BinaryOp : public Instruction {
      public:
        BinaryOp(const Local& destination, const RValue& left, const RValue& right, BinaryOpKind operation) :
            Instruction(), m_destination(destination), m_left(left), m_right(right), m_operation(operation) {}

        void print(std::ostream& out) const override {
            out << m_destination << " := " << m_left << " " << m_operation << " " << m_right;
        }

        void accept(Visitor& visitor) override;

        const Local& destination() const { return m_destination; }
        const RValue& left() const { return m_left; }
        const RValue& right() const { return m_right; }
        BinaryOpKind operation() const { return m_operation; }

      private:
        Local m_destination;
        RValue m_left;
        RValue m_right;
        BinaryOpKind m_operation;
    };

    enum class UnaryOpKind {
        MINUS,
        NOT,
    };

    inline std::ostream& operator<<(std::ostream& out, const UnaryOpKind& op) {
        switch (op) {
            case UnaryOpKind::MINUS: return out << "-";
            case UnaryOpKind::NOT: return out << "!";
        }
        return out;
    }

    // Instruction of the form "destination := op operand " like "a := -b"
    class UnaryOp : public Instruction {
      public:
        UnaryOp(const Local& destination, const RValue& operand, UnaryOpKind operation) :
            Instruction(), m_destination(destination), m_operand(operand), m_operation(operation) {}

        void print(std::ostream& out) const override { out << m_destination << " := " << m_operation << m_operand; }

        void accept(Visitor& visitor) override;

        const Local& destination() const { return m_destination; }
        const RValue& operand() const { return m_operand; }
        UnaryOpKind operation() const { return m_operation; }

      private:
        Local m_destination;
        RValue m_operand;
        UnaryOpKind m_operation;
    };

    // Instruction of the form "destination := source"
    class Assignment : public Instruction {
      public:
        Assignment(const Local& destination, const RValue& source) :
            Instruction(), m_destination(destination), m_source(source) {}

        void print(std::ostream& out) const override { out << m_destination << " := " << m_source; }

        const Local& destination() const { return m_destination; }

        const RValue& source() const { return m_source; }

        void accept(Visitor& visitor) override;

      private:
        Local m_destination;
        RValue m_source;
    };

    class Call : public Instruction {
      public:
        Call(const Local& destination, std::string name, std::vector<RValue> args) :
            Instruction(), m_destination(destination), m_name(std::move(name)), m_args(std::move(args)) {}

        void print(std::ostream& out) const override {
            out << m_destination << " := " << m_name << "(";
            bool first = true;
            for (auto& arg : m_args) {
                if (!first) out << ", ";
                first = false;
                out << arg;
            }
            out << ")";
        }

        const Local& destination() const { return m_destination; }

        const auto& name() const { return m_name; }

        const auto& args() const { return m_args; }

        void accept(Visitor& visitor) override;

      private:
        Local m_destination;
        std::string m_name;
        std::vector<RValue> m_args;
    };
}

template <>
struct std::hash<ir::Local> {
    size_t operator()(const ir::Local& local) const { return std::hash<uint32_t>{}(local.id()); }
};
