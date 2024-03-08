#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <utility>

class SymbolTable {
  public:
    bool contains(const std::string& symbol) const { return m_symbolTable.find(symbol) != m_symbolTable.end(); }

    int operator[](const std::string& symbol) const { return m_symbolTable.at(symbol); }

    void declare(std::string&& symbol) {
        uint32_t index = nextIndex;
        nextIndex += 4;
        m_symbolTable.insert({std::move(symbol), index});
    }

    void declare(const std::string& symbol) {
        uint32_t index = nextIndex;
        nextIndex += 4;
        m_symbolTable.insert({symbol, index});
    }

    auto begin() { return m_symbolTable.begin(); }

    auto end() { return m_symbolTable.end(); }

    auto size() const { return m_symbolTable.size(); }

  private:
    std::map<std::string, int> m_symbolTable;
    int nextIndex = 4;
};
