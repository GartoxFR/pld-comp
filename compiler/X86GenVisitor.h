#pragma once

#include "ir/Instructions.h"
#include "ir/Ir.h"
#include <ostream>

enum class Register : uint8_t {
    RAX = 0,
    RBX = 1,
    RCX = 2,
    RDX = 3,
    RSI = 4,
    RDI = 5,
    RSP = 6,
    RDP = 7,
    R8 = 8,
    R9 = 9,
    R10 = 10,
    R11 = 11,
    R12 = 12,
    R13 = 13,
    R14 = 14,
    R15 = 15,
};

struct SizedRegister {
    Register reg;
    size_t size;
};

struct Deref {
    SizedRegister reg;
};

struct Label {
    std::string_view label;
};

inline const std::string_view REGISTER_ALIASES[16][4]{
    {"%rax", "%eax", "%ax", "%al"},      {"%rbx", "%ebx", "%bx", "%bl"},      {"%rcx", "%ecx", "%cx", "%cl"},
    {"%rdx", "%edx", "%dx", "%dl"},      {"%rsi", "%esi", "%si", "%sil"},      {"%rdi", "%edi", "%di", "%dil"},
    {"%rsp", "%esp", "%sp", "%spl"},     {"%rdp", "%edp", "%dp", "%dpl"},     {"%r8", "%r8d", "%r8w", "%r8b"},
    {"%r9", "%r9d", "%r9w", "%r9b"},     {"%r10", "%r10d", "%r10w", "%r10b"}, {"%r11", "%r11d", "%r11w", "%r11b"},
    {"%r12", "%r12d", "%r12w", "%r12b"}, {"%r13", "%r13d", "%r13w", "%r13b"}, {"%r14", "%r14d", "%r14w", "%r14b"},
    {"%r15", "%r15d", "%r15w", "%r15b"},
};

inline std::string_view registerLabel(SizedRegister reg) {
    switch (reg.size) {
        case 8: return REGISTER_ALIASES[uint8_t(reg.reg)][0];
        case 4: return REGISTER_ALIASES[uint8_t(reg.reg)][1];
        case 2: return REGISTER_ALIASES[uint8_t(reg.reg)][2];
        case 1: return REGISTER_ALIASES[uint8_t(reg.reg)][3];
        default: throw std::runtime_error("Unsupported size");
    }
}

inline std::string_view getSuffix(size_t size) {
    switch (size) {
        case 8: return "q";
        case 4: return "l";
        case 2: return "w";
        case 1: return "b";
        default: throw std::runtime_error("Unsupported size");
    }
}

class X86GenVisitor : public ir::Visitor {
  public:
    X86GenVisitor(std::ostream& out) : m_out(out) {}

    void visit(ir::Function& function) override;
    void visit(ir::BasicBlock& block) override;
    void visit(ir::BinaryOp& binaryOp) override;
    void visit(ir::UnaryOp& unaryOp) override;
    void visit(ir::Assignment& assignment) override;
    void visit(ir::BasicJump& jump) override;
    void visit(ir::ConditionalJump& jump) override;
    void visit(ir::Call& call) override;
    void visit(ir::Cast& cast) override;
    void visit(ir::PointerRead& pointerRead) override;
    void visit(ir::PointerWrite& pointerWrite) override;
    void visit(ir::AddressOf& addressOf) override;

    void emitSimpleArithmetic(std::string_view instr, const ir::BinaryOp& binaryOp);
    void emitCmp(std::string_view instruction, const ir::BinaryOp& binaryOp);
    void emitDiv(bool modulo, const ir::BinaryOp& binaryOp);

    template <class... Ts>
    void emit(std::string_view instr, std::string_view suffix, Ts&&... args) {
        m_out << "    " << instr << suffix << "\t";
        emitArgs(args...);
        m_out << "\n";
    }

    template <class... Ts>
    void emit(std::string_view instr, Ts&&... args) {
        m_out << "    " << instr << "\t";
        if constexpr (sizeof...(Ts))
            emitArgs(args...);
        m_out << "\n";
    }

    template <class T, class... Ts>
    void emitArgs(T&& arg, Ts&&... args) {
        emitArg(std::forward<T>(arg));
        m_out << ", ";
        emitArgs(std::forward<Ts>(args)...);
    }

    template <class T, class... Ts>
        requires(sizeof...(Ts) == 0)
    void emitArgs(T&& arg) {
        emitArg(std::forward<T>(arg));
    }

    void emitArg(ir::Local arg) { m_out << variableLocation(arg); }

    void emitArg(ir::RValue arg) {
        if (std::holds_alternative<ir::Local>(arg)) {
            emitArg(std::get<ir::Local>(arg));
        } else {
            emitArg(std::get<ir::Immediate>(arg));
        }
    }

    void emitArg(ir::Immediate arg) {
        switch (arg.type()->size()) {
            case 8: m_out << "$" << arg.value64(); break;
            case 4: m_out << "$" << arg.value32(); break;
            case 2: m_out << "$" << arg.value16(); break;
            case 1: m_out << "$" << (int16_t) arg.value8(); break;
        }
    }

    void emitArg(const SizedRegister& arg) { m_out << registerLabel(arg); }

    void emitArg(const Deref& arg) { m_out << "(" << registerLabel(arg.reg) << ")"; }

    void emitArg(const Label& arg) { m_out << arg.label; }

  private:
    std::ostream& m_out;

    void loadEax(const ir::Local& local) { m_out << "    movl    " << variableLocation(local) << ", %eax\n"; }
    void loadEax(const ir::Immediate& immediate) { m_out << "    movl    $" << immediate.value32() << ", %eax\n"; }

    void loadEax(const ir::RValue& rvalue) {
        if (std::holds_alternative<ir::Local>(rvalue)) {
            loadEax(std::get<ir::Local>(rvalue));
        } else {
            loadEax(std::get<ir::Immediate>(rvalue));
        }
    }

    void saveEax(const ir::Local& local) { m_out << "    movl    %eax, " << variableLocation(local) << "\n"; }

    std::string variableLocation(const ir::Local& local) {
        std::stringstream ss;
        ss << "-" << (local.id() + 1) * 8 << "(%rbp)";
        return ss.str();
    }
};
