#include "InterferenceGraph.h"

#include <algorithm>

void InterferenceGraph::addInterference(LocalId a, LocalId b) {
    if (a == b) return;

    if (std::find(m_interferences[a].begin(), m_interferences[a].end(), b) == m_interferences[a].end()) {
        m_interferences[a].push_back(b);
    }

    if (std::find(m_interferences[b].begin(), m_interferences[b].end(), a) == m_interferences[b].end()) {
        m_interferences[b].push_back(a);
    }
}

void InterferenceGraph::printDot(std::ostream& out) const {
    out << "graph ig {" << std::endl;
    size_t i = 0;
    for (const auto& list : m_interferences) {
        out << "_" << i << std::endl;
        for (auto other : list) {
            if (i < other)
                out << "_" << i << " -- " << "_" << other << std::endl;
        }
        i++;
    }
    out << "}" << std::endl;
}
