#pragma once

#include "BlockLivenessAnalysis.h"
#include "InterferenceGraph.h"
#include "PointedLocalGatherer.h"
#include "ir/Instructions.h"
#include "ir/Ir.h"
#include <ostream>
#include <set>
#include <sstream>
#include <unordered_map>

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

enum class Register : uint8_t {
    RAX = 0,
    RBX = 1,
    RCX = 2,
    RDX = 3,
    RSI = 4,
    RDI = 5,
    RSP = 6,
    RBP = 7,
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

struct DerefOffset {
    SizedRegister reg;
    int offset;
};

struct Label {
    std::string_view label;
};

struct FunctionLabel {
    std::string_view label;
};

struct DataLabel {
    std::string_view label;
};

inline const std::string_view REGISTER_ALIASES[16][4]{
    {"%rax", "%eax", "%ax", "%al"},      {"%rbx", "%ebx", "%bx", "%bl"},      {"%rcx", "%ecx", "%cx", "%cl"},
    {"%rdx", "%edx", "%dx", "%dl"},      {"%rsi", "%esi", "%si", "%sil"},     {"%rdi", "%edi", "%di", "%dil"},
    {"%rsp", "%esp", "%sp", "%spl"},     {"%rbp", "%ebp", "%bp", "%bpl"},     {"%r8", "%r8d", "%r8w", "%r8b"},
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

inline bool preserved(Register reg) {
    switch (reg) {
        case Register::RBX:
        case Register::RSP:
        case Register::RBP:
        case Register::R12:
        case Register::R13:
        case Register::R14:
        case Register::R15: return true;

        case Register::RAX:
        case Register::RSI:
        case Register::RDI:
        case Register::RDX:
        case Register::RCX:
        case Register::R8:
        case Register::R9:
        case Register::R10:
        case Register::R11: return false;
    }

    return false;
}

class X86GenVisitor : public ir::Visitor {
  public:
    X86GenVisitor(std::ostream& out, bool doRegisterAllocation) :
        m_out(out), m_doRegisterAllocation(doRegisterAllocation) {}

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
    void emitSimpleArithmeticCommutative(std::string_view instr, const ir::BinaryOp& binaryOp);
    void emitCmp(std::string_view instruction, const ir::BinaryOp& binaryOp);
    void emitDiv(bool modulo, const ir::BinaryOp& binaryOp);

    template <class... Ts>
    void emit(std::string_view instr, std::string_view suffix, Ts&&... args) {
        m_currentInstruction = &m_instructions.emplace_back();
        std::stringstream ss;
        ss << instr << suffix;
        m_currentInstruction->push_back(ss.str());

        if constexpr (sizeof...(args) > 0)
            emitArgs(args...);
    }

    template <class... Ts>
    void emit(std::string_view instr, Ts&&... args) {
        m_currentInstruction = &m_instructions.emplace_back();
        m_currentInstruction->push_back(std::string(instr));

        if constexpr (sizeof...(Ts) > 0)
            emitArgs(args...);
    }

    template <class T, class... Ts>
    void emitArgs(T&& arg, Ts&&... args) {
        emitArg(std::forward<T>(arg));

        if constexpr (sizeof...(args) > 0)
            emitArgs(std::forward<Ts>(args)...);
    }

    void emitArg(ir::Local arg) { m_currentInstruction->push_back(variableLocation(arg)); }

    void emitArg(ir::RValue arg) {
        if (std::holds_alternative<ir::Local>(arg)) {
            emitArg(std::get<ir::Local>(arg));
        } else {
            emitArg(std::get<ir::Immediate>(arg));
        }
    }

    void emitArg(ir::Immediate arg) {
        std::stringstream ss;
        switch (arg.type()->size()) {
            case 8: ss << "$" << arg.value64(); break;
            case 4: ss << "$" << arg.value32(); break;
            case 2: ss << "$" << arg.value16(); break;
            case 1: ss << "$" << (int16_t)arg.value8(); break;
        }

        m_currentInstruction->push_back(ss.str());
    }

    void emitArg(const SizedRegister& arg) { m_currentInstruction->push_back(std::string(registerLabel(arg))); }

    void emitArg(const Deref& arg) {
        std::stringstream ss;
        ss << "(" << registerLabel(arg.reg) << ")";
        m_currentInstruction->push_back(ss.str());
    }

    void emitArg(const DerefOffset& arg) {
        std::stringstream ss;
        ss << arg.offset << "(" << registerLabel(arg.reg) << ")";
        m_currentInstruction->push_back(ss.str());
    }

    void emitArg(const Label& arg) { m_currentInstruction->push_back(std::string(arg.label)); }

    void emitArg(const FunctionLabel& arg) {
        std::stringstream ss;
        ss << arg.label << "@PLT";
        m_currentInstruction->push_back(ss.str());
    }

    void emitArg(const DataLabel& arg) {
        std::stringstream ss;
        ss << arg.label << "(%rip)";
        m_currentInstruction->push_back(ss.str());
    }

    void simplifyAsm();

    void printAsm();

    std::set<Register> registerAllocation(
        const ir::Function& function, const PointedLocals& pointerLocals, const InterferenceGraph& interferenceGraph
    );

  private:
    std::ostream& m_out;
    ir::Function* m_currentFunction;
    ir::BasicBlock* m_currentBlock;
    std::unordered_map<ir::Local, Register> m_localsInRegister;
    std::unordered_map<ir::Local, size_t> m_localsOnStack;
    BlockLivenessAnalysis m_livenessAnalysis;
    LocalsUsedThroughCalls m_localsUsedThroughCalls;
    std::optional<std::string_view> m_optimizedCond;

    std::vector<std::vector<std::string>> m_instructions;
    std::vector<std::string>* m_currentInstruction;

    // Number of 8 byte pushed on the stack since the call
    size_t m_stackAlignment = 0;

    bool m_doRegisterAllocation;

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
        auto it = m_localsInRegister.find(local);
        if (it != m_localsInRegister.end()) {
            ss << registerLabel(SizedRegister{it->second, local.type()->size()});
        } else {
            ss << "-" << m_localsOnStack.at(local) << "(%rbp)";
        }

        return ss.str();
    }

    std::optional<Register> variableRegister(const ir::Local& local) {
        auto it = m_localsInRegister.find(local);
        if (it != m_localsInRegister.end()) {
            return it->second;
        }

        return {};
    }

    std::optional<Register> rvalueRegister(const ir::RValue& rvalue) {
        return std::visit(
            overloaded{
                [&](ir::Local local) { return variableRegister(local); },
                [&](auto unused) { return std::optional<Register>{}; },
            },
            rvalue
        );
    }

    std::string literalLabel(const ir::StringLiteral& literal) {
        std::stringstream ss;
        ss << "." << m_currentFunction->name() << ".literal." << literal.id();
        return ss.str();
    }
};
