#pragma once

#include "shader_dasm.h"
#include <memory>
#include <vector>
#include <array>

namespace ShaderRewriter {
    enum class FunctionName {
        vec4,
        mul, add, abs, neg, ternary,
        fract, floor, dot2, lessThan,
        cast_float,
        gt, ge, eq, ne, lt, le
    };
    
    class IExpressionVisitor;
    
    class Expression {
    public:
        virtual ~Expression();
        virtual void accept(IExpressionVisitor* visitor) = 0;
    };
    
    class Variable : public Expression {
    public:
        std::string name();
    };
    
    class AttributeReference : public Variable {
        input_attr_t _attr;
        persp_corr_t _persp;
    public:
        AttributeReference(input_attr_t attr, persp_corr_t persp);
        input_attr_t attribute();
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class RegisterReference : public Variable {
        reg_type_t _reg;
        bool _isC;
        int _n;
    public:
        RegisterReference(reg_type_t reg, bool isC, int n);
        reg_type_t reg();
        int num();
        bool isC();
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
        IfStatement(std::unique_ptr<Expression> expr, 
                    std::vector<std::unique_ptr<Statement>> statements);
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
                       std::unique_ptr<Expression> left,
                       std::unique_ptr<Expression> right);
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class TernaryOperator : public Invocation {
        std::unique_ptr<Expression> _cond;
        std::unique_ptr<Expression> _trueBranch;
        std::unique_ptr<Expression> _falseBranch;
    public:
        TernaryOperator(std::unique_ptr<Expression> cond,
                        std::unique_ptr<Expression> trueBranch,
                        std::unique_ptr<Expression> falseBranch);
    };
    
    class BasicBlock {
    public:
        std::vector<Statement*> statements;
    };
    
    class IExpressionVisitor {
    public:
        virtual void visit(FloatLiteral* literal) = 0;
        virtual void visit(BinaryOperator* op) = 0;
        virtual void visit(UnaryOperator* op) = 0;
        virtual void visit(Swizzle* swizzle) = 0;
        virtual void visit(Assignment* assignment) = 0;
        virtual void visit(AttributeReference* ref) = 0;
        virtual void visit(RegisterReference* ref) = 0;
        virtual void visit(Invocation* invocation) = 0;
        virtual void visit(ComponentMask* mask) = 0;
        virtual void visit(IfStatement* mask) = 0;
    };
    
    std::vector<std::unique_ptr<Statement>> MakeStatement(FragmentInstr const& i);
    std::string PrintStatement(Statement* stat);
    int GetLastRegisterNum(Expression* expr);
}
