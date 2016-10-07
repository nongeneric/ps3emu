#pragma once

#include "shader_dasm.h"
#include <memory>
#include <vector>
#include <array>

namespace ShaderRewriter {
    enum class FunctionName {
        vec4, vec3, vec2, ivec4, equal, greaterThan, greaterThanEqual, lessThanEqual, notEqual,
        mul, div, add, abs, neg, exp2, lg2, min, max, pow, inversesqrt,
        fract, floor, ceil, dot, lessThan, cos, sin,
        cast_float, cast_int, clamp4i, clamp, sign, normalize,
        gt, ge, eq, ne, lt, le, mix, any,
        reverse4f, reverse3f, reverse2f,
        call, ret,
        txl0, txl1, txl2, txl3,
        ftex, ftxp, ftxb, ftxd, ftxl,
        floatBitsToUint, unpackUnorm4x8,
        none
    };
    
    class IExpressionVisitor;
    
    class Expression {
    public:
        virtual ~Expression();
        virtual void accept(IExpressionVisitor* visitor) = 0;
    };
    
    class Variable : public Expression {
        std::string _name;
        Expression* _index;
        
        Variable(std::string name, Expression* index);
        friend class ASTContext;
        
    public:
        std::string name();
        Expression* index();
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class Statement : public Expression {
        uint32_t _address;
    public:
        Statement();
        ~Statement();
        uint32_t address();
        void address(uint32_t value);
    };
    
    class Assignment : public Statement {
        Expression* _expr;
        Expression* _dest;
        
        Assignment(Expression* dest, Expression* expr);
        friend class ASTContext;
        
    public:
        Expression* expr();
        Expression* dest();
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class IfStatement : public Statement {
        Expression* _expr;
        std::vector<Statement*> _trueBlock;
        std::vector<Statement*> _falseBlock;
    
    protected:
        IfStatement(Expression* expr, 
                    std::vector<Statement*> trueBlock,
                    std::vector<Statement*> falseBlock,
                    unsigned address);
        friend class ASTContext;
        
    public: 
        std::vector<Statement*> trueBlock();
        std::vector<Statement*> falseBlock();
        void setTrueBlock(std::vector<Statement*> block);
        void setFalseBlock(std::vector<Statement*> block);
        Expression* condition();
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class IfStubFragmentStatement : public IfStatement {
        unsigned _elseLabel;
        unsigned _endifLabel;
        
        inline IfStubFragmentStatement(Expression* expr,
                                       unsigned elseLabel,
                                       unsigned endifLabel)
            : IfStatement(expr, {}, {}, 0), _elseLabel(elseLabel), _endifLabel(endifLabel) {}
        friend class ASTContext;
        
    public:
        inline unsigned elseLabel() { return _elseLabel; }
        inline unsigned endifLabel() { return _endifLabel; }
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class SwitchStatement : public Statement {
    public:
        struct Case {
            uint32_t address = -1;
            std::vector<Statement*> body;
        };
        
        void addCase(uint32_t address, std::vector<Statement*> body);
        std::vector<Case>& cases();
        Expression* switchOn();
        virtual void accept(IExpressionVisitor* visitor) override;
    private:
        SwitchStatement(Expression* switchOn);
        std::vector<Case> _cases;
        Expression* _switchOn;
        friend class ASTContext;
    };
    
    class WhileStatement : public Statement {
        Expression* _condition;
        std::vector<Statement*> _body;
        WhileStatement(Expression* condition, std::vector<Statement*> body);
        friend class ASTContext;
    public:
        Expression* condition();
        std::vector<Statement*> body();
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class BreakStatement : public Statement {
        BreakStatement() = default;
        friend class ASTContext;
        
    public:
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class DiscardStatement : public Statement {
        DiscardStatement() = default;
        friend class ASTContext;
        
    public:
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class FloatLiteral : public Expression {
        float _val;
        FloatLiteral(float val);
        friend class ASTContext;
        
    public:
        float value();
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class IntegerLiteral : public Expression {
        int _val;
        
        IntegerLiteral(int val);
        friend class ASTContext;
        
    public:
        int value();
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class Swizzle : public Expression {
        swizzle_t _swizzle;
        Expression* _expr;
        
        Swizzle(Expression* expr, swizzle_t _swizzle);
        friend class ASTContext;
        
    public:
        Expression* expr();
        swizzle_t swizzle();
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class ComponentMask : public Expression {
        Expression* _expr;
        dest_mask_t _mask;
        
        ComponentMask(Expression* expr, dest_mask_t mask);
        friend class ASTContext;
        
    public:
        Expression* expr();
        dest_mask_t& mask();
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class Invocation : public Expression {
        std::vector<Expression*> _args;
        FunctionName _name;
        
    protected:
        template<typename... P>
        Invocation(FunctionName name, P&&... ps) : _args{ps...}, _name(name) {}
        friend class ASTContext;
        
    public:
        FunctionName name();
        std::vector<Expression*> args();
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class UnaryOperator : public Invocation {
        friend class ASTContext;
        UnaryOperator(FunctionName name, Expression* arg);
        
    public:
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class BinaryOperator : public Invocation {
        friend class ASTContext;
        BinaryOperator(FunctionName name,
                       Expression* left,
                       Expression* right);
        
    public:
        virtual void accept(IExpressionVisitor* visitor) override;
    };
    
    class TernaryOperator : public Invocation {
        friend class ASTContext;
        TernaryOperator(Expression* cond,
                        Expression* trueExpr,
                        Expression* falseExpr);
        
    public:
        virtual void accept(IExpressionVisitor* visitor) override;
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
        virtual void visit(IfStatement* st) = 0;
        virtual void visit(IfStubFragmentStatement* st) = 0;
        virtual void visit(SwitchStatement* sw) = 0;
        virtual void visit(BreakStatement* be) = 0;
        virtual void visit(DiscardStatement* be) = 0;
        virtual void visit(WhileStatement* we) = 0;
        virtual void visit(TernaryOperator* we) = 0;
    };
        
    using ExprV = std::vector<Expression*>;
    using StV = std::vector<Statement*>;
    
    class ASTContext {
        std::vector<Expression*> _exprs;
    public:
        template <typename E, typename... P>
        E* make(P&&... args) {
            auto expr = new E(std::forward<P>(args)...);
            _exprs.emplace_back(expr);
            return expr;
        }
    };
}
