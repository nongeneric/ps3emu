#pragma once

#include "AST.h"
#include "DefaultVisitor.h"
#include "shader_dasm.h"
#include <memory>
#include <vector>
#include <array>
#include <set>

namespace ShaderRewriter {
    std::vector<Statement*> MakeStatement(ASTContext& context,
                                          FragmentInstr const& i,
                                          unsigned constIndex);
    std::vector<Statement*> MakeStatement(ASTContext& context,
                                          VertexInstr const& i,
                                          unsigned address);
    std::vector<Statement*> RewriteBranches(ASTContext& context,
                                            std::vector<Statement*> statements);
    std::vector<Statement*> RewriteIfStubs(ASTContext& context,
                                           std::vector<Statement*> statements);
    std::string PrintStatement(Statement* stat);
    std::tuple<int, int> GetLastRegisterNum(Expression* expr);
    
    class UsedConstsVisitor : public DefaultVisitor {
        std::set<unsigned> _consts;
    public:
        virtual void visit(Variable* variable) override;
        std::set<unsigned> const& consts();
    };
}
