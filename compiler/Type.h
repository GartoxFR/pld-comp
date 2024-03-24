#pragma once

#include <iostream>
#include <stdexcept>
#include <unordered_map>

class Type {
  public:
    Type(std::string name, size_t size) : m_size(size), m_target(nullptr), m_name(std::move(name)) {}
    Type(std::string name, const Type* target) : m_size(8), m_target(target), m_name(std::move(name)) {}

    bool isPtr() const { return m_target != nullptr; }
    size_t size() const { return m_size; }
    auto target() const { return m_target; }
    const auto& name() const { return m_name; }

  private:
    size_t m_size;
    const Type* m_target;
    std::string m_name;
};

namespace types {
    inline Type* INT;
    inline Type* CHAR;
    inline Type* LONG;
    inline Type* BOOL;
    inline Type* SHORT;
    inline Type* VOID;
}

class TypePool {
  public:
    void init() {
        types::INT = &m_simpleTypes.insert({"int", Type("int", 4)}).first->second;
        types::CHAR = &m_simpleTypes.insert({"char", Type("char", 1)}).first->second;
        types::BOOL = &m_simpleTypes.insert({"bool", Type("bool", 1)}).first->second;
        types::LONG = &m_simpleTypes.insert({"long", Type("long", 8)}).first->second;
        types::SHORT = &m_simpleTypes.insert({"short", Type("short", 2)}).first->second;
        types::VOID = &m_simpleTypes.insert({"void", Type("void", size_t(0))}).first->second;
    }

  private:
    std::unordered_map<std::string, Type> m_simpleTypes;
    std::unordered_map<const Type*, Type> m_pointerTypes;

    friend const Type* make_simple_type(const std::string& name);
    friend const Type* make_pointer_type(const Type* target);
};

inline TypePool globalTypePool;

inline const Type* make_simple_type(const std::string& name) {
    auto it = globalTypePool.m_simpleTypes.find(name);
    if (it == globalTypePool.m_simpleTypes.end()) {
        throw std::runtime_error("Unknown type");
    }

    return &it->second;
}

inline const Type* make_pointer_type(const Type* type) {
    auto it = globalTypePool.m_pointerTypes.find(type);
    if (it != globalTypePool.m_pointerTypes.end()) {
        return &it->second;
    }

    std::string name = type->name() + "*";
    return &globalTypePool.m_pointerTypes.insert({type, Type(std::move(name), type)}).first->second;
}
