#include "AST.h"

namespace ShaderRewriter {
    FloatLiteral::FloatLiteral(float val) : _val(val) { }

    Invocation::Invocation(FunctionName name, std::vector<Expression*> args)
        : _name(name) 
    {
        for (auto arg : args) {
            _args.emplace_back(arg);
        }
    }

    UnaryOperator::UnaryOperator(FunctionName name, Expression* arg)
        : Invocation(name, { arg }) { }

    Swizzle::Swizzle(Expression* expr, swizzle_t swizzle) 
        : _swizzle(swizzle), _expr(expr) { }

    Assignment::Assignment(Expression* dest, Expression* expr)
        : _expr(expr), _dest(dest) { }

    Expression::~Expression() { }

    BinaryOperator::BinaryOperator(FunctionName name,
                                Expression* left,
                                Expression* right) 
    : Invocation(name, { left, right }) { }

    void Variable::accept(IExpressionVisitor* visitor) { visitor->visit(this); }
    void Assignment::accept(IExpressionVisitor* visitor) { visitor->visit(this); }
    void FloatLiteral::accept(IExpressionVisitor* visitor) { visitor->visit(this); }
    void Swizzle::accept(IExpressionVisitor* visitor) { visitor->visit(this); }
    void Invocation::accept(IExpressionVisitor* visitor) { visitor->visit(this); }
    void UnaryOperator::accept(IExpressionVisitor* visitor) { visitor->visit(this); }
    void BinaryOperator::accept(IExpressionVisitor* visitor) { visitor->visit(this); }
    void ComponentMask::accept(IExpressionVisitor* visitor) { visitor->visit(this); }
    void IfStatement::accept(IExpressionVisitor* visitor) { visitor->visit(this); }
    void IntegerLiteral::accept(IExpressionVisitor* visitor) { visitor->visit(this); }

    float FloatLiteral::value() {
        return _val;
    }

    FunctionName Invocation::name() {
        return _name;
    }

    std::vector<Expression*> Invocation::args() {
        std::vector<Expression*> exprs;
        for (auto& expr : _args)
            exprs.push_back(expr.get());
        return exprs;
    }

    Expression* Swizzle::expr() {
        return _expr.get();
    }

    swizzle_t Swizzle::swizzle() {
        return _swizzle;
    }

    Expression* Assignment::expr() {
        return _expr.get();
    }

    Expression* Assignment::dest() {
        return _dest.get();
    }
    
    ComponentMask::ComponentMask(Expression* expr, dest_mask_t mask)
        : _expr(expr), _mask(mask) { }
    
    Expression* ComponentMask::expr() {
        return _expr.get();
    }
    
    dest_mask_t ComponentMask::mask() {
        return _mask;
    }
    
    TernaryOperator::TernaryOperator(Expression* cond, 
                                     Expression* trueBranch,
                                     Expression* falseBranch)
        : Invocation(FunctionName::ternary, { }),
          _cond(cond), 
          _trueBranch(trueBranch),
          _falseBranch(falseBranch) { }
    
    IfStatement::IfStatement(Expression* expr,
                             std::vector<Statement*> statements)
        : _expr(expr)
    {
        for (auto st : statements) {
            _statements.emplace_back(st);
        }
    }
    
    std::vector<Statement*> IfStatement::statements() {
        std::vector<Statement*> res;
        for (auto& st : _statements)
            res.push_back(st.get());
        return res;
    }
    
    Expression* IfStatement::condition() {
        return _expr.get();
    }
    
    void Assignment::releaseAndReplaceExpr(Expression* expr) {
        _expr.release();
        _expr.reset(expr);
    }
    
    void Invocation::releaseAndReplaceArg(int n, Expression* expr) {
        _args.at(n).release();
        _args.at(n).reset(expr);
    }
    
    IntegerLiteral::IntegerLiteral(int val) : _val(val) { }
    
    int IntegerLiteral::value() {
        return _val;
    }
    
    Variable::Variable(std::string name, Expression* index) 
        : _name(name), _index(index) { }
    
    std::string Variable::name() {
        return _name;
    }
    
    Expression* Variable::index() {
        return _index.get();
    }
}