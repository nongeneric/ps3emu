#pragma once

#include "AST.h"
#include "shader_dasm.h"
#include <memory>
#include <vector>
#include <array>

namespace ShaderRewriter {
    std::vector<std::unique_ptr<Statement>> MakeStatement(FragmentInstr const& i, unsigned constIndex);
    std::vector<std::unique_ptr<Statement>> MakeStatement(VertexInstr const& i, unsigned address);
    std::string PrintStatement(Statement* stat);
    int GetLastRegisterNum(Expression* expr);
}
