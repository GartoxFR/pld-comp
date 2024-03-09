#pragma once

#include "tree/ParseTree.h"
#include <cstdint>
#include <ostream>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <utility>

// Represent a string ident that is stored inside the SymbolTable
//
// Invariants : - m_symbol must point to a string stored inside the SymbolTable
//              - To Symbol representing identical string must have m_symbol which points to the same place
class Ident {
  public:
    // Trivial conversion to a string_view
    operator std::string_view() const { return m_value; }

    // Pointer comparison is fine due to the invariants
    bool operator==(const Ident& other) const {
        return m_value.data() == other.m_value.data() && m_value.size() == other.m_value.size();
    }

    // Only compare the pointer and sizes
    auto operator<=>(const Ident& other) const {
        auto cmp = m_value <=> other.m_value.data();
        if (cmp != std::strong_ordering::equivalent)
            return cmp;

        return m_value.size() <=> other.m_value.size();
    }

  private:
    // This constructor is private so that only the symbol table can give us these instances
    // This helps ensure the invariants holds
    explicit Ident(std::string_view symbol) : m_value(symbol) {}

    std::string_view m_value;

    friend class SymbolTable;
    friend struct std::hash<Ident>;

    friend inline std::ostream &operator<<(std::ostream& out, const Ident& symbol) {
        return out << symbol.m_value;
    }
};

// Define how to hash a Symbol. We can simply hash the pointers
template <>
struct std::hash<Ident> {
    size_t operator()(const Ident& symbol) const {
        return std::hash<const char*>()(symbol.m_value.data()) ^ std::hash<size_t>()(symbol.m_value.size());
    }
};

class SymbolTable {
  public:
    bool contains(const std::string& identStr) {
        Ident ident = toIdent(identStr);
        return contains(ident);
    }

    bool contains(const Ident& ident) const {
        return m_symbolTable.find(ident) != m_symbolTable.end();
    }

    int operator[](const std::string& identStr) {
        Ident ident = toIdent(identStr);
        return operator[](ident);
    }

    int operator[](const Ident& symbol) const {
        return m_symbolTable.at(symbol);
    }

    void declare(std::string&& identStr) {
        Ident ident = toIdent(std::move(identStr));
        declare(ident);
    }

    void declare(const std::string& identStr) {
        Ident ident = toIdent(identStr);
        declare(ident);
    }

    void declare(const Ident& ident) {
        uint32_t index = nextIndex;
        nextIndex += 4;
        m_symbolTable.insert({ident, index});
    }

    Ident toIdent(const std::string& symbol) {
        // Inserting into a set returns the iterator to the inserted element. If the set already contains
        // the element, it just returns the contained element
        return Ident(*m_identPool.insert(symbol).first);
    }

    Ident toIdent(std::string&& symbol) {
        // Inserting into a set returns the iterator to the inserted element. If the set already contains
        // the element, it just returns the contained element
        return Ident(*m_identPool.insert(std::move(symbol)).first);
    }

    auto begin() { return m_symbolTable.begin(); }

    auto end() { return m_symbolTable.end(); }

    auto size() const { return m_symbolTable.size(); }

  private:
    std::unordered_map<Ident, int> m_symbolTable;
    std::unordered_set<std::string> m_identPool;
    int nextIndex = 4;
};

inline SymbolTable globalSymbolTable;

inline Ident make_ident(antlr4::tree::ParseTree* node) {
    return globalSymbolTable.toIdent(node->getText());
}

