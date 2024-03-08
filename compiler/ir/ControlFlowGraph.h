#pragma once

#include "Instructions.h"
#include <memory>
#include <vector>

namespace ir {
    class Terminator;

    // A block in the ControlFlowGraph
    // It's composed of a list of Instruction and a Terminator
    // The Terminator is null if the block is the epilogue or if it's still in construction
    class BasicBlock {
      private:
        std::vector<std::unique_ptr<Instruction>> m_instructions;
        std::unique_ptr<Terminator> m_terminator;
    };

    class ControlFlowGraph {
      private:
        std::vector<std::unique_ptr<BasicBlock>> m_blocks;
    };
}
