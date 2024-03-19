#pragma once

#include "ir/Function.h"
#include "ir/Instructions.h"
#include <iostream>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>

class IrSymbolTable {
  public:
    bool hasErrors() const { return m_error; }

    void enterNewLocalScope() { m_localScopeStack.emplace_back(); }

    void exitLocalScope() { m_localScopeStack.pop_back(); }

    void declareLocalVariable(std::string str, ir::Local localVariable) {
        VariableScope& currentScope = m_localScopeStack.back();

        if (currentScope.contains(str)) {
            m_error = true;
            std::cerr << "Error: Variable " << str << " already declared in this scope." << std::endl;
            return;
        }

        currentScope.insert({std::move(str), localVariable});
    }

    ir::Local getLocalVariable(const std::string& str) {
        for (auto& scope : m_localScopeStack | std::views::reverse) {
            auto it = scope.find(str);
            if (it != scope.end()) {
                return it->second;
            }
        }

        std::cerr << "Error: Variable " << str << " not declared in this scope." << std::endl;
        m_error = true;

        // We return a Local but the IR won't be valid anyway since we have undeclared variables
        return ir::Local{INT32_MAX};
    }

    void declareFunction(const ir::Function& function) {

        if (m_functions.contains(function.name())) {
            m_error = true;
            std::cerr << "Error: Function " << function.name() << " already declared in this scope." << std::endl;
            return;
        }

        m_functions.insert({function.name(), &function});
    }

    bool checkFunction(const std::string& name, size_t argCount) {
        auto it = m_functions.find(name);
        if (it == m_functions.end()) {
            m_error = true;
            return false;
        }

        if (it->second->argCount() != argCount) {
            m_error = true;
            return false;
        }

        return true;
    }

  private:
    using VariableScope = std::unordered_map<std::string, ir::Local>;

    std::vector<VariableScope> m_localScopeStack;
    std::unordered_map<std::string, const ir::Function*> m_functions;
    bool m_error = false;
};
