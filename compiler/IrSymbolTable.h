#pragma once

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

    void declareLocalVariable(const std::string& str, ir::Local localVariable) {
        VariableScope& currentScope = m_localScopeStack.back();

        if (currentScope.contains(str)) {
            m_error = true;
            std::cerr << "Error: Variable " << str << " already declared in this scope." << std::endl;
            return;
        }

        currentScope.insert({str, localVariable});
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

  private:
    using VariableScope = std::unordered_map<std::string, ir::Local>;

    std::vector<VariableScope> m_localScopeStack;
    bool m_error = false;
};
