#pragma once

#include "Instructions.h"
#include <memory>
#include <vector>
#include <sstream>

namespace ir {
    class Terminator;
    class BasicBlock;

    // A block in the ControlFlowGraph
    // It's composed of a list of Instruction and a Terminator
    // The Terminator is null if the block is the epilogue or if it's still in construction
    class BasicBlock : public Visitable {
      public:
        BasicBlock(const std::string& functionName, uint32_t id) {
            std::stringstream labelStream;
            labelStream << "." << functionName << ".BB" << id; 
            m_label = labelStream.str();
        }

        BasicBlock(const std::string& customLabel) : m_label(customLabel) {
        }

        // Append an instruction of type InstructionT to the block and return a pointer to it.
        // Example : emit<Copy>(destination, source) to add a Copy instruction
        template <class InstructionT, class... Args>
            requires std::derived_from<InstructionT, Instruction>
        InstructionT* emit(Args&&... args) {
            // Create a unique pointer to the Instruction
            std::unique_ptr<InstructionT> unique = std::make_unique<InstructionT>(std::forward<Args>(args)...);

            // Get a basic pointer from it to return it
            InstructionT* ptr = unique.get();

            // Move our unique pointer to the vector.
            // std::move is required because unique_ptr can't be copied as this would result
            // in use after free errors
            m_instructions.push_back(std::move(unique));
            return ptr;
        }

        // Terminate the block by constructing a Terminator of type TerminatorT with
        // the supplied args
        template <class TerminatorT, class... Args>
            requires std::derived_from<TerminatorT, Terminator>
        TerminatorT* terminate(Args&&... args) {
            // Create a unique_ptr to the actual type
            std::unique_ptr<TerminatorT> unique = std::make_unique<TerminatorT>(std::forward<Args>(args)...);

            // Get the pointer from it
            TerminatorT* ptr = unique.get();

            // Move the pointer to the member, upcasting it
            m_terminator = std::move(unique);

            return ptr;
        }

        void terminate(std::unique_ptr<Terminator>&& terminator) {
            m_terminator = std::move(terminator);
        }

        const std::string& label() const { return m_label; }

        auto& instructions() { return m_instructions; }
        const auto& instructions() const { return m_instructions; }

        auto& terminator() { return m_terminator; }
        const auto& terminator() const { return m_terminator; }

        void accept(Visitor& visitor) override;

        friend inline std::ostream& operator<<(std::ostream& out, const BasicBlock& self) {
            return out << "BasicBlock(label = " << self.label() << "instructionCount = " << self.instructions().size()
                       << "terminated = " << bool(self.terminator()) << ")";
        }

      private:
        std::vector<std::unique_ptr<Instruction>> m_instructions;
        std::unique_ptr<Terminator> m_terminator;

        // Unique label of the block
        std::string m_label;
    };
}
