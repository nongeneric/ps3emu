#pragma once

#include <vector>
#include <string>
#include <tuple>

class NinjaScript {
    std::vector<std::string> _variables;
    std::vector<std::string> _rules;
    std::vector<std::string> _subninjas;
    std::vector<std::string> _statements;
    std::vector<std::string> _defaults;
    
public:
    void rule(std::string name, std::string command);
    void statement(std::string rule,
                   std::string in,
                   std::string out,
                   std::vector<std::tuple<std::string, std::string>> variables);
    void variable(std::string name, std::string value);
    void subninja(std::string name);
    void defaultStatement(std::string name);
    std::string dump();
};
