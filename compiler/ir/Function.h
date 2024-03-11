#pragma once

#include "BasicBlock.h"
#include "../SymbolTable.h"

namespace ir {

    // Informations about a local variable
    class LocalInfo {
      public:
        LocalInfo() = default;
        explicit LocalInfo(Ident m_name) : m_name(m_name) {}

        const std::optional<Ident>& name() const { return m_name; }

        bool isTemporary() const { return !m_name.has_value(); }

      private:
        // Name for user defined variables, null for temporaries
        std::optional<Ident> m_name;
    };

    // Represents a function in a form of a ControlFlowGraph
    class Function {
      public:
        Function(const Ident& name, size_t argCount) : m_name(name), m_argCount(argCount) {
            // Push info for temporary return value variable
            m_locals.emplace_back();
        }

        // Allocate a new BasicBlock for this function
        BasicBlock* newBlock() {
            m_blocks.push_back(std::make_unique<BasicBlock>(m_name, m_blocks.size()));
            return m_blocks.back().get();
        }

        // Allocate a new temporary local variable in this Function
        Local newLocal() {
            uint32_t id = m_locals.size();
            m_locals.emplace_back();
            return Local{id};
        }

        // Allocate a new named local variable in this Function
        Local newLocal(const Ident& ident) {
            uint32_t id = m_locals.size();
            m_locals.emplace_back(ident);
            return Local{id};
        }

        const auto& name() const { return m_name; }

        size_t argCount() const { return m_argCount; }

        void printLocalMapping(std::ostream& out) const {
            out << "debug {" << std::endl;
            for (size_t i = 0; i < m_locals.size(); i++) {
                if (!m_locals[i].isTemporary())
                    out << std::format("    _{} => {}", i, std::string_view(m_locals[i].name().value())) << std::endl;
            }
            out << "}" << std::endl;
        }

      private:
        // The function name
        Ident m_name;

        // Number of arguments this function has
        size_t m_argCount;

        // Information about all local variables. Always at least (m_argCount + 1) elements : the return variable _0 and the arguments _1 to _m_argCount
        std::vector<LocalInfo> m_locals;

        // Allocated on the heap to not invalidate pointers to BasicBlocks when resizing the vector
        std::vector<std::unique_ptr<BasicBlock>> m_blocks;
    };
}
