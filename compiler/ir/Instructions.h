#pragma once

#include "../Type.h"
#include "Visitable.h"
#include <cassert>
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
        explicit Local(uint32_t id, const Type* type) : m_id(id), m_type(type) {}

        uint32_t id() const { return m_id; }

        auto type() const { return m_type; }

        bool operator==(const Local& other) const { return m_id == other.m_id; }

        void setType(const Type* type) {
            m_type = type;
        }

        std::strong_ordering operator<=>(const Local& other) const { return m_id <=> other.m_id; }

        friend inline std::ostream& operator<<(std::ostream& out, const Local& self) { return out << "_" << self.id(); }

      private:
        uint32_t m_id;
        const Type* m_type;
    };

    // An immediate value (constant like 5 or 42)
    class Immediate {
      public:
        explicit Immediate(int64_t value, const Type* type) : m_value(value), m_type(type) { assert(type != nullptr); }

        int32_t value32() const { return m_value; }
        int64_t value64() const { return m_value; }
        int8_t value8() const { return m_value; }
        int16_t value16() const { return m_value; }

        auto type() const { return m_type; }

        void setType(const Type* type) {
            m_type = type;
        }

        bool operator==(const Immediate& immediate) const { return immediate.m_value == m_value; }

        friend inline std::ostream& operator<<(std::ostream& out, const Immediate& self) {
            return out << self.value32();
        }

      private:
        int64_t m_value;
        const Type* m_type;
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
        MOD,
        EQ,
        NEQ,
        CMP_L,
        CMP_G,
        CMP_LE,
        CMP_GE,
        BIT_AND,
        BIT_XOR,
        BIT_OR,
    };

    inline std::ostream& operator<<(std::ostream& out, const BinaryOpKind& op) {
        switch (op) {
            case BinaryOpKind::ADD: return out << "+";
            case BinaryOpKind::SUB: return out << "-";
            case BinaryOpKind::MUL: return out << "*";
            case BinaryOpKind::DIV: return out << "/";
            case BinaryOpKind::MOD: return out << "%";
            case BinaryOpKind::EQ: return out << "==";
            case BinaryOpKind::NEQ: return out << "!=";
            case BinaryOpKind::CMP_L: return out << "<";
            case BinaryOpKind::CMP_G: return out << ">";
            case BinaryOpKind::CMP_LE: return out << "<=";
            case BinaryOpKind::CMP_GE: return out << ">=";
            case BinaryOpKind::BIT_AND: return out << "&";
            case BinaryOpKind::BIT_XOR: return out << "^";
            case BinaryOpKind::BIT_OR: return out << "|";
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
        Local& destination() { return m_destination; }
        const RValue& left() const { return m_left; }
        RValue& left() { return m_left; }
        const RValue& right() const { return m_right; }
        RValue& right() { return m_right; }
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
        Local& destination() { return m_destination; }
        const RValue& operand() const { return m_operand; }
        RValue& operand() { return m_operand; }
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
        Local& destination() { return m_destination; }

        const RValue& source() const { return m_source; }
        RValue& source() { return m_source; }

        void accept(Visitor& visitor) override;

      private:
        Local m_destination;
        RValue m_source;
    };

    class Call : public Instruction {
      public:
        Call(const Local& destination, std::string name, std::vector<RValue> args, bool variadic = false) :
            Instruction(), m_destination(destination), m_name(std::move(name)), m_args(std::move(args)), m_variadic(variadic) {}

        void print(std::ostream& out) const override {
            out << m_destination << " := " << m_name << "(";
            bool first = true;
            for (auto& arg : m_args) {
                if (!first)
                    out << ", ";
                first = false;
                out << arg;
            }
            out << ")";
        }

        const Local& destination() const { return m_destination; }
        Local& destination() { return m_destination; }

        const auto& name() const { return m_name; }

        const auto& args() const { return m_args; }
        auto& args() { return m_args; }

        bool variadic() const { return m_variadic; }

        void accept(Visitor& visitor) override;

      private:
        Local m_destination;
        std::string m_name;
        std::vector<RValue> m_args;
        bool m_variadic;
    };

    class Cast : public Instruction {
      public:
        Cast(Local m_destination, RValue m_source) : m_destination(m_destination), m_source(m_source) {}

        const Local& destination() const { return m_destination; }
        Local& destination() { return m_destination; }

        const RValue& source() const { return m_source; }
        RValue& source() { return m_source; }

        void accept(Visitor& visitor) override;

        void print(std::ostream& out) const override {
            out << m_destination << " := (" << m_destination.type()->name() << ") " << m_source;
        }

      private:
        Local m_destination;
        RValue m_source;
    };

    class PointerRead : public Instruction {
      public:
        PointerRead(Local m_destination, Local m_address) : m_destination(m_destination), m_address(m_address) {}

        const Local& destination() const { return m_destination; }
        Local& destination() { return m_destination; }

        const RValue& address() const { return m_address; }
        RValue& address() { return m_address; }

        void accept(Visitor& visitor) override;

        void print(std::ostream& out) const override { out << m_destination << " := *" << m_address; }

      private:
        Local m_destination;
        RValue m_address;
    };

    class PointerWrite : public Instruction {
      public:
        PointerWrite(Local m_address, RValue m_source) : m_address(m_address), m_source(m_source) {}

        const RValue& address() const { return m_address; }
        RValue& address() { return m_address; }

        const RValue& source() const { return m_source; }
        RValue& source() { return m_source; }

        void accept(Visitor& visitor) override;

        void print(std::ostream& out) const override { out << "*" << m_address << " := " << m_source; }

      private:
        RValue m_address;
        RValue m_source;
    };

    class StringLiteral {
      public:
        explicit StringLiteral(uint32_t m_id) : m_id(m_id) {}

        const auto id() const { return m_id; }

        bool operator==(const StringLiteral& other) const { return m_id == other.m_id; }

        std::strong_ordering operator<=>(const StringLiteral& other) const { return m_id <=> other.m_id; }

        friend inline std::ostream& operator<<(std::ostream& out, const StringLiteral& self) { return out << "_literal_" << self.id(); }
      private:
        uint32_t m_id;
    };

    using Addressable = std::variant<Local, StringLiteral>;

    inline std::ostream& operator<<(std::ostream& out, const Addressable& addr) {
        std::visit([&](auto val) { out << val; }, addr);
        return out;
    }

    class AddressOf : public Instruction {
      public:
        AddressOf(Local m_destination, Addressable m_source) : m_destination(m_destination), m_source(m_source) {}

        const Local& destination() const { return m_destination; }
        Local& destination() { return m_destination; }

        const Addressable& source() const { return m_source; }
        Addressable& source() { return m_source; }

        void accept(Visitor& visitor) override;

        void print(std::ostream& out) const override { out << m_destination << " := &" << m_source; }

      private:
        Local m_destination;
        Addressable m_source;
    };
}

template <>
struct std::hash<ir::Local> {
    size_t operator()(const ir::Local& local) const { return std::hash<uint32_t>{}(local.id()); }
};
