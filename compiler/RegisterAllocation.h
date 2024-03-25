#pragma once

#include "InterferenceGraph.h"
#include "PointedLocalGatherer.h"
#include "ir/Ir.h"
#include <unordered_map>
#include <unordered_set>

using RegisterId = uint32_t;

class RegisterAllocationResult {
  public:
    const auto& registers() const { return m_registers; }
    const auto& spilled() const { return m_spilled; }

  private:
    std::unordered_set<LocalId> m_spilled;
    std::unordered_map<LocalId, RegisterId> m_registers;

    RegisterAllocationResult(const PointedLocals& poitedLocals) {
        std::transform(
            poitedLocals.begin(), poitedLocals.end(), std::inserter(m_spilled, m_spilled.begin()),
            [](ir::Local local) { return local.id(); }
        );
    }

    friend RegisterAllocationResult computeRegisterAllocation(
        const ir::Function& function, const PointedLocals& pointedLocals, const InterferenceGraph& interferenceGraph,
        uint32_t registerCount
    );
};

RegisterAllocationResult computeRegisterAllocation(
    const ir::Function& function, const PointedLocals& pointedLocals, const InterferenceGraph& interferenceGraph,
    uint32_t registerCount
);
