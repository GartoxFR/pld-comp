#include "RegisterAllocation.h"
#include <ranges>
#include <set>

RegisterAllocationResult computeRegisterAllocation(
    const ir::Function& function, const PointedLocals& pointedLocals, const InterferenceGraph& interferenceGraph,
    uint32_t registerCount
) {
    constexpr uint32_t minRegister = 0;
    auto range = std::ranges::iota_view(minRegister, registerCount);
    const std::set<LocalId> possibleRegisters{range.begin(), range.end()} ;

    RegisterAllocationResult result{pointedLocals};
    std::vector<LocalId> removedVertexStack;

    auto isInGraph = [&](LocalId local) {
        return !result.m_spilled.contains(local) &&
            std::ranges::find(removedVertexStack, local) == removedVertexStack.end();
    };

    bool found = true;
    while (found) {
        found = false;
        LocalId minLocal;
        size_t minNeighbors = 0;
        for (LocalId local = 0; local < interferenceGraph.localCount(); local++) {
            if (!isInGraph(local))
                continue;

            size_t neighbors = std::ranges::count_if(interferenceGraph.neighbors(local), isInGraph);
            if (!found || minNeighbors < neighbors) {
                minNeighbors = neighbors;
                minLocal = local;
                found = true;
            }
        }

        if (found) {
            removedVertexStack.push_back(minLocal);
        }
    }

    while (!removedVertexStack.empty()) {
        LocalId local = removedVertexStack.back();
        removedVertexStack.pop_back();

        std::set<LocalId> possibleColors = possibleRegisters;
        for (auto neighbor : interferenceGraph.neighbors(local)) {
            auto it = result.m_registers.find(neighbor);
            if (it != result.m_registers.end()) {
                possibleColors.erase(it->second);
            }
        }

        if (possibleColors.empty()) {
            result.m_spilled.insert(local);
        } else {
            result.m_registers.insert({local, *possibleColors.begin()});
        }
    }

    return result;
}
