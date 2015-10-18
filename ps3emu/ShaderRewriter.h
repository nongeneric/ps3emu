#pragma once

#include "shader_dasm.h"
#include <memory>
#include <vector>
#include <array>

namespace ShaderRewriter {
    enum class FunctionName {
        vec4,
        mul, abs, neg
    };
    
    class IExpressionVisitor;
    
    class Expression {
    public:
        virtual ~Expression();
        virtual std::string accept(IExpressionVisitor* visitor) = 0;
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
        ~AttributeReference();
        input_attr_t attribute();
        virtual std::string accept(IExpressionVisitor* visitor) override;
    };
    
    class RegisterReference : public Variable {
        reg_type_t _reg;
        bool _isC;
        int _n;
    public:
        RegisterReference(reg_type_t reg, bool isC, int n);
        ~RegisterReference();
        reg_type_t reg();
        int num();
        bool isC();
        virtual std::string accept(IExpressionVisitor* visitor) override;
    };
    
    class Statement : public Expression {
        
    };
    
    class Assignment : public Statement {
        std::unique_ptr<Expression> _expr;
        std::unique_ptr<Expression> _dest;
    public:
        Assignment(Expression* dest, Expression* expr);
        ~Assignment();
        Expression* expr();
        Expression* dest();
        virtual std::string accept(IExpressionVisitor* visitor) override;;
    };
    
    class Literal : public Expression {
        
    };
    
    class FloatLiteral : public Literal {
        float _val;
    public:
        FloatLiteral(float val);
        ~FloatLiteral();
        float value();
        virtual std::string accept(IExpressionVisitor* visitor) override;
    };
    
    class Swizzle : public Expression {
        swizzle_t _swizzle;
        std::unique_ptr<Expression> _expr;
    public:
        Swizzle(Expression* expr, swizzle_t _swizzle);
        ~Swizzle();
        Expression* expr();
        swizzle_t swizzle();
        virtual std::string accept(IExpressionVisitor* visitor) override;
    };
    
    class ComponentMask : public Expression {
        std::unique_ptr<Expression> _expr;
        dest_mask_t _mask;
    public:
        ComponentMask(Expression* expr, dest_mask_t mask);
        ~ComponentMask();
        Expression* expr();
        dest_mask_t mask();
        virtual std::string accept(IExpressionVisitor* visitor) override;
    };
    
    class Invocation : public Expression {
        std::vector<std::unique_ptr<Expression>> _args;
        FunctionName _name;
    public:
        Invocation(FunctionName name, std::vector<Expression*> args);
        ~Invocation();
        FunctionName name();
        std::vector<Expression*> args();
        virtual std::string accept(IExpressionVisitor* visitor) override;
    };
    
    class UnaryOperator : public Invocation {
    public:
        UnaryOperator(FunctionName name, Expression* arg);
        ~UnaryOperator();
        virtual std::string accept(IExpressionVisitor* visitor) override;
    };
    
    class BinaryOperator : public Invocation {
    public:
        BinaryOperator(FunctionName name,
                       std::unique_ptr<Expression> left,
                       std::unique_ptr<Expression> right);
        ~BinaryOperator();
        virtual std::string accept(IExpressionVisitor* visitor) override;
    };
    
    class BasicBlock {
    public:
        std::vector<Statement*> statements;
    };
    
    class IExpressionVisitor {
    public:
        virtual std::string visit(FloatLiteral* literal) = 0;
        virtual std::string visit(BinaryOperator* op) = 0;
        virtual std::string visit(UnaryOperator* op) = 0;
        virtual std::string visit(Swizzle* swizzle) = 0;
        virtual std::string visit(Assignment* assignment) = 0;
        virtual std::string visit(AttributeReference* ref) = 0;
        virtual std::string visit(RegisterReference* ref) = 0;
        virtual std::string visit(Invocation* invocation) = 0;
        virtual std::string visit(ComponentMask* mask) = 0;
    };
    
    std::unique_ptr<Statement> MakeStatement(FragmentInstr const& i);
    std::string PrintStatement(Statement* stat);
}
