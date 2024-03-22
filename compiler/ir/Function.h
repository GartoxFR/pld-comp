#pragma once

#include "BasicBlock.h"
#include "Visitable.h"
#include <algorithm>
#include <optional>
#include <sstream>

namespace ir {

    // Informations about a local variable
    class LocalInfo {
      public:
        explicit LocalInfo(const Type* type) : m_type(type) {}
        LocalInfo(std::string m_name, const Type* type) : m_name(std::move(m_name)), m_type(type) {}

        const std::optional<std::string>& name() const { return m_name; }

        auto type() const { return m_type; }

        bool isTemporary() const { return !m_name.has_value(); }

      private:
        // Name for user defined variables, null for temporaries
        std::optional<std::string> m_name;
        const Type* m_type;
    };

    // Represents a function in a form of a ControlFlowGraph
    class Function : public Visitable {
      public:
        Function(const std::string& name, size_t argCount, const Type* returnType) :
            m_name(name), m_argCount(argCount), m_locals({LocalInfo(returnType)}),
            m_prologue(generatePrologueLabel(name)), m_epilogue(generateEpilogueLabel(name)) {}

        Function(const std::string& name, std::initializer_list<const Type*> argTypes, const Type* returnType) :
            m_name(name), m_argCount(argTypes.size()), m_prologue(generatePrologueLabel(name)),
            m_epilogue(generateEpilogueLabel(name)) {
            m_locals.emplace_back(returnType);
            std::transform(std::begin(argTypes), std::end(argTypes), std::back_inserter(m_locals), [](auto type) {
                return LocalInfo(type);
            });
        }

        // Allocate a new BasicBlock for this function
        BasicBlock* newBlock() {
            m_blocks.push_back(std::make_unique<BasicBlock>(m_name, m_blocks.size()));
            return m_blocks.back().get();
        }

        BasicBlock* prologue() { return &m_prologue; }
        BasicBlock* epilogue() { return &m_epilogue; }

        // Allocate a new temporary local variable in this Function
        Local newLocal(const Type* type) {
            uint32_t id = m_locals.size();
            m_locals.emplace_back(type);
            return Local{id, type};
        }

        // Allocate a new named local variable in this Function
        Local newLocal(const std::string& ident, const Type* type) {
            uint32_t id = m_locals.size();
            m_locals.emplace_back(ident, type);
            return Local{id, type};
        }

        Local returnLocal() const { return Local{0, m_locals.at(0).type()}; }

        const auto& name() const { return m_name; }

        size_t argCount() const { return m_argCount; }

        const auto& blocks() const { return m_blocks; }
        auto& blocks() { return m_blocks; }

        const auto& locals() const { return m_locals; }
        auto& locals() { return m_locals; }

        void printLocalMapping(std::ostream& out) const {
            out << "debug " << m_name << " {" << std::endl;
            for (size_t i = 0; i < m_locals.size(); i++) {
                if (!m_locals[i].isTemporary())
                    out << "    _" << i << " => " << m_locals[i].name().value() << std::endl;
            }
            out << "}" << std::endl;
        }

        void accept(Visitor& visitor) override;

      private:
        // The function name
        std::string m_name;

        // Number of arguments this function has
        size_t m_argCount;

        // Information about all local variables. Always at least (m_argCount + 1) elements : the return variable _0 and the arguments _1 to _m_argCount
        std::vector<LocalInfo> m_locals;

        // Allocated on the heap to not invalidate pointers to BasicBlocks when resizing the vector
        std::vector<std::unique_ptr<BasicBlock>> m_blocks;

        BasicBlock m_prologue;
        BasicBlock m_epilogue;

        static std::string generatePrologueLabel(const std::string& functionName) {
            std::stringstream ss;
            ss << "." << functionName << ".prologue";
            return ss.str();
        }

        static std::string generateEpilogueLabel(const std::string& functionName) {
            std::stringstream ss;
            ss << "." << functionName << ".epilogue";
            return ss.str();
        }
    };
}
