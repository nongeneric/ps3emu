#pragma once

#include "AST.h"

namespace ShaderRewriter {

    class DefaultVisitor : public IExpressionVisitor {
    public:
        virtual void visit(FloatLiteral* literal) override;
        virtual void visit(BinaryOperator* op) override;
        virtual void visit(UnaryOperator* op) override;
        virtual void visit(Swizzle* swizzle) override;
        virtual void visit(Variable* ref) override;
        virtual void visit(IntegerLiteral* ref) override;
        virtual void visit(ComponentMask* mask) override;
        virtual void visit(IfStatement* ifst) override;
        virtual void visit(IfStubFragmentStatement* ifst) override;
        virtual void visit(Assignment* assignment) override;
        virtual void visit(Invocation* invocation) override;
        virtual void visit(SwitchStatement* sw) override;
        virtual void visit(BreakStatement* sw) override;
        virtual void visit(DiscardStatement* sw) override;
        virtual void visit(WhileStatement* sw) override;
        virtual void visit(TernaryOperator* ternary) override;
    };
}