#pragma once

#include <cstdint>
#include <ostream>
#include <vector>

using LocalId = uint32_t;

class InterferenceGraph {
  public:
    explicit InterferenceGraph(size_t localCount) : m_interferences(localCount) {}

    void addInterference(LocalId a, LocalId b);

    const auto& neighbors(LocalId a) const { return m_interferences[a]; }

    size_t localCount() const { return m_interferences.size(); }

    void printDot(std::ostream& out) const;

  private:
    std::vector<std::vector<LocalId>> m_interferences;
};

