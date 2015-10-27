#pragma once

#include "shader_dasm.h"
#include <memory>
#include <vector>
#include <array>

namespace ShaderRewriter {
    enum class FunctionName {
        vec4, equal, greaterThan, greaterThanEqual, lessThanEqual, notEqual,
        mul, add, abs, neg, ternary, exp2, lg2, min, max, pow, inversesqrt,
        fract, floor, ceil, dot2, dot3, dot4, lessThan, cos, sin,
        cast_float, clamp4i, sign,
        gt, ge, eq, ne, lt, le,
        reverse4f,
        branch, call, ret
    };
    
    class IExpressionVisitor;
    
    class Expression {
    public:
        virtual ~Expression();
        virtual void accept(IExpressionVisitor* visitor) = 0;
    };
    
    class Variable : public Expression {
        std::string _name;
        std::unique_ptr<Expression> _index;
    public:
        Variable(std::string name, Expression* index);
        std::string name();
        Expression* index();
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class Statement : public Expression {
        
    };
    
    class Assignment : public Statement {
        std::unique_ptr<Expression> _expr;
        std::unique_ptr<Expression> _dest;
    public:
        Assignment(Expression* dest, Expression* expr);
        Expression* expr();
        Expression* dest();
        void releaseAndReplaceExpr(Expression* expr);
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class IfStatement : public Statement {
        std::unique_ptr<Expression> _expr;
        std::vector<std::unique_ptr<Statement>> _statements;
    public:
        IfStatement(Expression* expr, 
                    std::vector<Statement*> statements);
        std::vector<Statement*> statements();
        Expression* condition();
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class FloatLiteral : public Expression {
        float _val;
    public:
        FloatLiteral(float val);
        float value();
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class IntegerLiteral : public Expression {
        int _val;
    public:
        IntegerLiteral(int val);
        int value();
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class Swizzle : public Expression {
        swizzle_t _swizzle;
        std::unique_ptr<Expression> _expr;
    public:
        Swizzle(Expression* expr, swizzle_t _swizzle);
        Expression* expr();
        swizzle_t swizzle();
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class ComponentMask : public Expression {
        std::unique_ptr<Expression> _expr;
        dest_mask_t _mask;
    public:
        ComponentMask(Expression* expr, dest_mask_t mask);
        Expression* expr();
        dest_mask_t mask();
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class Invocation : public Expression {
        std::vector<std::unique_ptr<Expression>> _args;
        FunctionName _name;
    public:
        Invocation(FunctionName name, std::vector<Expression*> args);
        FunctionName name();
        std::vector<Expression*> args();
        void releaseAndReplaceArg(int n, Expression* expr);
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class UnaryOperator : public Invocation {
    public:
        UnaryOperator(FunctionName name, Expression* arg);
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class BinaryOperator : public Invocation {
    public:
        BinaryOperator(FunctionName name,
                       Expression* left,
                       Expression* right);
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class TernaryOperator : public Invocation {
        std::unique_ptr<Expression> _cond;
        std::unique_ptr<Expression> _trueBranch;
        std::unique_ptr<Expression> _falseBranch;
    public:
        TernaryOperator(Expression* cond,
                        Expression* trueBranch,
                        Expression* falseBranch);
    };
    
    class BasicBlock {
    public:
        std::vector<Statement*> statements;
    };
    
    class IExpressionVisitor {
    public:
        virtual void visit(FloatLiteral* literal) = 0;
        virtual void visit(IntegerLiteral* literal) = 0;
        virtual void visit(BinaryOperator* op) = 0;
        virtual void visit(UnaryOperator* op) = 0;
        virtual void visit(Swizzle* swizzle) = 0;
        virtual void visit(Assignment* assignment) = 0;
        virtual void visit(Variable* ref) = 0;
        virtual void visit(Invocation* invocation) = 0;
        virtual void visit(ComponentMask* mask) = 0;
        virtual void visit(IfStatement* mask) = 0;
    };
}