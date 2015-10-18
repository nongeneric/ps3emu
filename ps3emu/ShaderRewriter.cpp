#include "ShaderRewriter.h"
#include <map>
#include <vector>
#include <algorithm>
#include "utils.h"

namespace ShaderRewriter {
    
    int getSingleComponent(swizzle_t s) {
        if (s.comp[0] == s.comp[1] &&
            s.comp[1] == s.comp[2] &&
            s.comp[2] == s.comp[3])
            return (int)s.comp[0];
        return -1;
    }
    
    std::unique_ptr<Expression> ConvertArgument(FragmentInstr const& i, int n) {
        assert(n < i.opcode.op_count);
        auto& arg = i.arguments[n];
        assert(arg.is_abs + arg.is_neg <= 1);
        Expression* expr = nullptr;
        bool ignoreSwizzle = false;
        if (arg.type == op_type_t::Const) {
            auto singleComp = getSingleComponent(arg.swizzle);
            if (singleComp != -1) {
                expr = new FloatLiteral(*(float*)&arg.imm_val[singleComp]);
                ignoreSwizzle = true;
            } else {
                expr = new Invocation(
                    FunctionName::vec4,
                    {
                        new FloatLiteral(*(float*)&arg.imm_val[0]),
                        new FloatLiteral(*(float*)&arg.imm_val[1]),
                        new FloatLiteral(*(float*)&arg.imm_val[2]),
                        new FloatLiteral(*(float*)&arg.imm_val[3])
                    }
                );
            }
        }
        if (arg.type == op_type_t::Attr) {
            expr = new AttributeReference(i.intput_attr, i.persp_corr);
        }
        if (arg.type == op_type_t::Temp) {
            expr = new RegisterReference(arg.reg_type, false, arg.reg_num);
        }
        if (arg.is_abs) {
            expr = new UnaryOperator(FunctionName::abs, expr);
        }
        if (arg.is_neg) {
            expr = new UnaryOperator(FunctionName::neg, expr);
        }
        if (!ignoreSwizzle) {
            if (arg.swizzle.comp[0] != swizzle2bit_t::X ||
                arg.swizzle.comp[1] != swizzle2bit_t::Y ||
                arg.swizzle.comp[2] != swizzle2bit_t::Z ||
                arg.swizzle.comp[3] != swizzle2bit_t::W) {
                expr = new Swizzle(expr, arg.swizzle);
            }
        }
        return std::unique_ptr<Expression>(expr);
    }
    
    std::unique_ptr<Statement> MakeStatement(FragmentInstr const& i) {
        std::vector<fragment_op_t> nops = {
            fragment_op_t::NOP,
            fragment_op_t::FENCB,
            fragment_op_t::FENCT
        };
        
        if (std::find(begin(nops), end(nops), i.opcode.instr) != end(nops)) {
            return nullptr;
        }
        
        std::map<fragment_op_t, FunctionName> operators = {
            { fragment_op_t::MUL, FunctionName::mul }
        };
        
        Statement* st = nullptr;
        auto it = operators.find(i.opcode.instr);
        if (it != end(operators)) {
            auto dest = new RegisterReference(i.reg, i.is_reg_c, i.reg_num);
            auto mask = new ComponentMask(dest, i.dest_mask);
            auto op = new BinaryOperator(
                it->second,
                ConvertArgument(i, 0),
                ConvertArgument(i, 1)
            );
            st = new Assignment(mask, op);
        }
        
        return std::unique_ptr<Statement>(st);
    }

    FloatLiteral::FloatLiteral(float val) : _val(val) { }
    
    FloatLiteral::~FloatLiteral() { }
    
    Invocation::Invocation(FunctionName name, std::vector<Expression*> args)
        : _name(name) 
    {
        for (auto arg : args) {
            _args.emplace_back(arg);
        }
    }
    
    Invocation::~Invocation() { }
    
    UnaryOperator::UnaryOperator(FunctionName name, Expression* arg)
        : Invocation(name, { arg }) { }
    
    UnaryOperator::~UnaryOperator() { }
    
    Swizzle::~Swizzle() { }
    
    Swizzle::Swizzle(Expression* expr, swizzle_t swizzle) 
        : _swizzle(swizzle), _expr(expr) { }

    AttributeReference::AttributeReference(input_attr_t attr, persp_corr_t persp)
        : _attr(attr), _persp(persp) { }

    AttributeReference::~AttributeReference() { }
    
    RegisterReference::RegisterReference(reg_type_t reg, bool isC, int n) 
        : _reg(reg), _isC(isC), _n(n) { }
    
    RegisterReference::~RegisterReference() { }
    
    Assignment::~Assignment() { }
    
    Assignment::Assignment(Expression* dest, Expression* expr)
        : _expr(expr), _dest(dest) { }
    
    BinaryOperator::~BinaryOperator() { }
    
    Expression::~Expression() { }
    
    BinaryOperator::BinaryOperator(FunctionName name,
                                   std::unique_ptr<Expression> left,
                                   std::unique_ptr<Expression> right) 
        : Invocation(name, { left.release(), right.release() }) { }
        
    std::string AttributeReference::accept(IExpressionVisitor* visitor) { return visitor->visit(this); }
    std::string RegisterReference::accept(IExpressionVisitor* visitor) { return visitor->visit(this); }
    std::string Assignment::accept(IExpressionVisitor* visitor) { return visitor->visit(this); }
    std::string FloatLiteral::accept(IExpressionVisitor* visitor) { return visitor->visit(this); }
    std::string Swizzle::accept(IExpressionVisitor* visitor) { return visitor->visit(this); }
    std::string Invocation::accept(IExpressionVisitor* visitor) { return visitor->visit(this); }
    std::string UnaryOperator::accept(IExpressionVisitor* visitor) { return visitor->visit(this); }
    std::string BinaryOperator::accept(IExpressionVisitor* visitor) { return visitor->visit(this); }
    std::string ComponentMask::accept(IExpressionVisitor* visitor) { return visitor->visit(this); }
    
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
    
    input_attr_t AttributeReference::attribute() {
        return _attr;
    }

    reg_type_t RegisterReference::reg() {
        return _reg;
    }
    
    int RegisterReference::num() {
        return _n;
    }
    
    class PrintVisitor : public IExpressionVisitor {
        std::string printOperatorName(FunctionName name) {
            switch (name) {
                case FunctionName::mul: return "*";
                default: assert(false); return "";
            }
        }
        
        virtual std::string visit(FloatLiteral* literal) override {
            return ssnprintf("%g", literal->value());
        }
        
        virtual std::string visit(BinaryOperator* op) override {
            auto name = printOperatorName(op->name());
            auto args = op->args();
            auto left = args.at(0)->accept(this);
            auto right = args.at(1)->accept(this);
            return ssnprintf("(%s %s %s)", left.c_str(), name.c_str(), right.c_str());
        }
        
        virtual std::string visit(UnaryOperator* op) override {
            auto name = printOperatorName(op->name());
            auto expr = op->args().at(0)->accept(this);
            return ssnprintf("(%s%s)", name.c_str(), expr.c_str());
        }
        
        virtual std::string visit(Swizzle* swizzle) override {
            auto expr = swizzle->expr()->accept(this);
            auto sw = print_swizzle(swizzle->swizzle());
            return ssnprintf("%s%s", expr.c_str(), sw.c_str());
        }
        
        virtual std::string visit(Assignment* assignment) override {
            auto var = assignment->dest()->accept(this);
            auto expr = assignment->expr()->accept(this);
            return ssnprintf("%s = %s;", var.c_str(), expr.c_str());
        }
        
        virtual std::string visit(AttributeReference* ref) override {
            auto attr = print_attr(ref->attribute());
            return ssnprintf("attr_%s", attr);
        }
        
        virtual std::string visit(RegisterReference* ref) override {
            auto reg = ref->reg() == reg_type_t::H ? "h" : "r";
            return ssnprintf("%s%d", reg, ref->num());
        }
        
        virtual std::string visit(Invocation* invocation) override {
            std::string str;
            auto args = invocation->args();
            for (auto i = 0u; i < args.size(); ++i) {
                if (i > 0)
                    str += ", ";
                auto arg = args.at(i)->accept(this);
                str += arg;
            }
            const char* name = nullptr;
            switch (invocation->name()) {
                case FunctionName::vec4:
                    name = "vec4";
                    break;
                default: assert(false);
            }
            return ssnprintf("%s(%s)", name, str.c_str());
        }
        
        virtual std::string visit(ComponentMask* mask) override {
            auto expr = mask->expr()->accept(this);
            auto strMask = print_dest_mask(mask->mask());
            return ssnprintf("%s%s", expr.c_str(), strMask.c_str());
        }
            
    };
    
    std::string PrintStatement(Statement* stat) {
        PrintVisitor visitor;
        return stat->accept(&visitor);
    }

    bool RegisterReference::isC() {
        return _isC;
    }
    
    ComponentMask::ComponentMask(Expression* expr, dest_mask_t mask)
        : _expr(expr), _mask(mask) { }
    
    ComponentMask::~ComponentMask() { }
    
    Expression* ComponentMask::expr() {
        return _expr.get();
    }
    
    dest_mask_t ComponentMask::mask() {
        return _mask;
    }
}
