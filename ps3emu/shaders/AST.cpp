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
    void IfStubFragmentStatement::accept(IExpressionVisitor* visitor) { visitor->visit(this); }
    void SwitchStatement::accept(IExpressionVisitor* visitor) { visitor->visit(this); }
    void IntegerLiteral::accept(IExpressionVisitor* visitor) { visitor->visit(this); }
    void BreakStatement::accept(IExpressionVisitor* visitor) { visitor->visit(this); }
    void DiscardStatement::accept(IExpressionVisitor* visitor) { visitor->visit(this); }
    void WhileStatement::accept(IExpressionVisitor* visitor) { visitor->visit(this); }
    void TernaryOperator::accept(IExpressionVisitor* visitor) { visitor->visit(this); }

    float FloatLiteral::value() {
        return _val;
    }

    FunctionName Invocation::name() {
        return _name;
    }

    std::vector<Expression*> Invocation::args() {
        return unpack_unique(_args);
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
    
    dest_mask_t& ComponentMask::mask() {
        return _mask;
    }
    
    TernaryOperator::TernaryOperator(Expression* cond, 
                                     Expression* trueExpr,
                                     Expression* falseExpr)
        : Invocation(FunctionName::none, { cond, trueExpr, falseExpr }) { }
    
    IfStatement::IfStatement(Expression* expr,
                             std::vector<Statement*> trueBlock,
                             std::vector<Statement*> falseBlock,
                             unsigned address)
        : _expr(expr),
          _trueBlock(pack_unique(trueBlock)),
          _falseBlock(pack_unique(falseBlock)) {
        this->address(address);
    }
    
    std::vector<Statement*> IfStatement::trueBlock() {
        return unpack_unique(_trueBlock);
    }
    
    std::vector<Statement*> IfStatement::falseBlock() {
        return unpack_unique(_falseBlock);
    }
    
    void IfStatement::setTrueBlock(std::vector<Statement*> block) {
        _trueBlock = pack_unique(block);
    }
    
    void IfStatement::setFalseBlock(std::vector<Statement*> block) {
        _falseBlock = pack_unique(block);
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
    
    Statement::Statement() : _address(-1) { }
    
    Statement::~Statement() = default;
    
    uint32_t Statement::address() {
        return _address;
    }
    
    void Statement::address(uint32_t value) {
        _address = value;
    }
    
    SwitchStatement::SwitchStatement(Expression* switchOn) : _switchOn(switchOn) { }
    
    void SwitchStatement::addCase(uint32_t address, std::vector<Statement*> body) {
        _cases.emplace_back(Case{ address, pack_unique(body) });
    }
 
    std::vector<SwitchStatement::Case>& SwitchStatement::cases() {
        return _cases;
    }
    
    Expression* SwitchStatement::switchOn() {
        return _switchOn.get();
    }
    
    void Expression::replace(Expression* what, Expression* with) {
        throw std::runtime_error("not implemented");
    }
    
    void Expression::replace(Expression* what, std::vector<Expression*> with) {
        throw std::runtime_error("not implemented");
    }
    
    Expression* Expression::release(Expression* expr) {
        throw std::runtime_error("not implemented");
    }
    
    WhileStatement::WhileStatement(Expression* condition,
                                   std::vector<Statement*> body)
        : _condition(condition), _body(pack_unique(body)) {}
 
    Expression* WhileStatement::condition() {
        return _condition.get();
    }
    
    std::vector<Statement*> WhileStatement::body() {
        return unpack_unique(_body);
    }
}