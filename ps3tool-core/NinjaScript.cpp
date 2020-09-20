#include "NinjaScript.h"

#include "ps3emu/utils.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>

using namespace boost::algorithm;

void NinjaScript::rule(std::string name, std::string command) {
    _rules.push_back("rule " + name);
    _rules.push_back("  command = " + command);
    _rules.push_back("");
}

void NinjaScript::statement(std::string rule,
                std::string in,
                std::string out,
                std::vector<std::tuple<std::string, std::string>> variables) {
    _statements.push_back(sformat("build {}: {} {}", out, rule, in));
    for (auto var : variables) {
        _statements.push_back(
            sformat("  {} = {}", std::get<0>(var), std::get<1>(var)));
    }
}

void NinjaScript::variable(std::string name, std::string value) {
    _variables.push_back(sformat("{} = {}", name, value));
    _variables.push_back("");
}

void NinjaScript::subninja(std::string name) {
    _subninjas.push_back(sformat("subninja {}", name));
}

void NinjaScript::defaultStatement(std::string name) {
    _defaults.push_back(sformat("default {}", name));
}

std::string NinjaScript::dump() {
    std::vector<std::string> script;
    std::copy(begin(_variables), end(_variables), std::back_inserter(script));
    script.push_back("");
    std::copy(begin(_rules), end(_rules), std::back_inserter(script));
    script.push_back("");
    std::copy(begin(_subninjas), end(_subninjas), std::back_inserter(script));
    script.push_back("");
    std::copy(begin(_statements), end(_statements), std::back_inserter(script));
    script.push_back("");
    std::copy(begin(_defaults), end(_defaults), std::back_inserter(script));
    script.push_back("");
    return join(script, "\n");
}
