#pragma once

#include "AST.h"
#include "DefaultVisitor.h"
#include "shader_dasm.h"
#include <memory>
#include <vector>
#include <array>
#include <set>

namespace ShaderRewriter {
    std::vector<std::unique_ptr<Statement>> MakeStatement(FragmentInstr const& i, unsigned constIndex);
    std::vector<std::unique_ptr<Statement>> MakeStatement(VertexInstr const& i, unsigned address);
    std::vector<std::unique_ptr<Statement>> RewriteBranches(
        std::vector<std::unique_ptr<Statement>> statements);
    std::vector<std::unique_ptr<Statement>> RewriteIfStubs(
        std::vector<std::unique_ptr<Statement>> statements);
    std::string PrintStatement(Statement* stat);
    int GetLastRegisterNum(Expression* expr);
    
    class UsedConstsVisitor : public DefaultVisitor {
        std::set<unsigned> _consts;
    public:
        virtual void visit(Variable* variable) override;
        std::set<unsigned> const& consts();
    };
}
