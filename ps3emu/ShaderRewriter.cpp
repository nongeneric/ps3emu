#include "ShaderRewriter.h"
#include <map>
#include <vector>
#include <algorithm>
#include "utils.h"

namespace ShaderRewriter {
    
    enum class ExprType {
        fp32, int32, boolean, ivec2, ivec3, ivec4,
        vec2, vec3, vec4, bvec2, bvec3, bvec4, notype
    };

    struct FunctionInfo {
        FunctionName name;
        ExprType types[5];
    };
    
    FunctionInfo functionInfos[] {
        { FunctionName::lessThan, 
            { ExprType::bvec4, ExprType::vec4, ExprType::vec4 } },
        { FunctionName::vec4, 
            { ExprType::vec4, ExprType::fp32, ExprType::fp32, ExprType::fp32, ExprType::fp32 } },
        { FunctionName::fract, { ExprType::notype, ExprType::notype } },
        { FunctionName::floor, { ExprType::notype, ExprType::notype } },
        { FunctionName::cast_float, { ExprType::fp32, ExprType::notype } },
        { FunctionName::dot2, { ExprType::fp32, ExprType::vec2, ExprType::vec2 } },
        { FunctionName::abs, { ExprType::notype, ExprType::notype } },
    };
    
    std::vector<fragment_op_t> nops = {
        fragment_op_t::NOP,
        fragment_op_t::FENCB,
        fragment_op_t::FENCT
    };
    
    std::map<fragment_op_t, FunctionName> operators = {
        { fragment_op_t::MUL, FunctionName::mul },
        { fragment_op_t::ADD, FunctionName::add },
    };
    
    std::map<fragment_op_t, FunctionName> functions = {
        { fragment_op_t::FRC, FunctionName::fract },
        { fragment_op_t::FLR, FunctionName::floor },
        { fragment_op_t::DP2, FunctionName::dot2 },
        { fragment_op_t::SLT, FunctionName::lessThan },
    };
    
    int getSingleComponent(swizzle_t s) {
        if (s.comp[0] == s.comp[1] &&
            s.comp[1] == s.comp[2] &&
            s.comp[2] == s.comp[3])
            return (int)s.comp[0];
        return -1;
    }
    
    FunctionName relationToFunction(cond_t relation) {
        switch (relation) {
            case cond_t::EQ:
            case cond_t::FL: return FunctionName::eq;
            case cond_t::GE: return FunctionName::ge;
            case cond_t::GT: return FunctionName::gt;
            case cond_t::LE: return FunctionName::le;
            case cond_t::LT: return FunctionName::lt;
            case cond_t::NE: return FunctionName::ne;
            default: assert(false); return FunctionName::ternary;
        }
    }

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

    AttributeReference::AttributeReference(input_attr_t attr, persp_corr_t persp)
        : _attr(attr), _persp(persp) { }
    
    RegisterReference::RegisterReference(reg_type_t reg, bool isC, int n) 
        : _reg(reg), _isC(isC), _n(n) 
    { 
        assert(!isC || n < 2);
    }
    
    Assignment::Assignment(Expression* dest, Expression* expr)
        : _expr(expr), _dest(dest) { }
        
    Expression::~Expression() { }
    
    BinaryOperator::BinaryOperator(FunctionName name,
                                   std::unique_ptr<Expression> left,
                                   std::unique_ptr<Expression> right) 
        : Invocation(name, { left.release(), right.release() }) { }
        
    void AttributeReference::accept(IExpressionVisitor* visitor) { visitor->visit(this); }
    void RegisterReference::accept(IExpressionVisitor* visitor) { visitor->visit(this); }
    void Assignment::accept(IExpressionVisitor* visitor) { visitor->visit(this); }
    void FloatLiteral::accept(IExpressionVisitor* visitor) { visitor->visit(this); }
    void Swizzle::accept(IExpressionVisitor* visitor) { visitor->visit(this); }
    void Invocation::accept(IExpressionVisitor* visitor) { visitor->visit(this); }
    void UnaryOperator::accept(IExpressionVisitor* visitor) { visitor->visit(this); }
    void BinaryOperator::accept(IExpressionVisitor* visitor) { visitor->visit(this); }
    void ComponentMask::accept(IExpressionVisitor* visitor) { visitor->visit(this); }
    void IfStatement::accept(IExpressionVisitor* visitor) { visitor->visit(this); }
    
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
        std::string _ret;
        
        std::string printOperatorName(FunctionName name) {
            switch (name) {
                case FunctionName::mul: return "*";
                case FunctionName::add: return "+";
                case FunctionName::neg: return "-";
                case FunctionName::gt: return ">";
                case FunctionName::ge: return ">=";
                case FunctionName::eq: return "==";
                case FunctionName::ne: return "!=";
                case FunctionName::lt: return "<";
                case FunctionName::le: return "<=";
                default: assert(false); return "";
            }
        }
        
        std::string accept(Expression* st) {
            st->accept(this);
            return _ret;
        }
        
        virtual void visit(FloatLiteral* literal) override {
            _ret = ssnprintf("%g", literal->value());
        }
        
        virtual void visit(BinaryOperator* op) override {
            auto name = printOperatorName(op->name());
            auto args = op->args();
            auto left = accept(args.at(0));
            auto right = accept(args.at(1));
            _ret = ssnprintf("(%s %s %s)", left.c_str(), name.c_str(), right.c_str());
        }
        
        virtual void visit(UnaryOperator* op) override {
            auto name = printOperatorName(op->name());
            auto expr = accept(op->args().at(0));
            _ret = ssnprintf("(%s%s)", name.c_str(), expr.c_str());
        }
        
        virtual void visit(Swizzle* swizzle) override {
            auto expr = accept(swizzle->expr());
            auto sw = print_swizzle(swizzle->swizzle(), true);
            _ret = ssnprintf("(%s%s)", expr.c_str(), sw.c_str());
        }
        
        virtual void visit(Assignment* assignment) override {
            auto var = accept(assignment->dest());
            auto expr = accept(assignment->expr());
            _ret = ssnprintf("%s = %s;", var.c_str(), expr.c_str());
        }
        
        virtual void visit(AttributeReference* ref) override {
            auto attr = print_attr(ref->attribute());
            _ret = ssnprintf("attr_%s", attr);
        }
        
        virtual void visit(RegisterReference* ref) override {
            auto reg = ref->reg() == reg_type_t::H ? "h" : "r";
            if (ref->isC())
                reg = "c";
            _ret = ssnprintf("%s%d", reg, ref->num());
        }
        
        virtual void visit(Invocation* invocation) override {
            std::string str;
            auto args = invocation->args();
            for (auto i = 0u; i < args.size(); ++i) {
                if (i > 0)
                    str += ", ";
                auto arg = accept(args.at(i));
                str += arg;
            }
            const char* name = nullptr;
            switch (invocation->name()) {
                case FunctionName::vec4: name = "vec4"; break;
                case FunctionName::fract: name = "fract"; break;
                case FunctionName::floor: name = "floor"; break;
                case FunctionName::dot2: name = "dot"; break;
                case FunctionName::lessThan: name = "lessThan"; break;
                case FunctionName::abs: name = "abs"; break;
                case FunctionName::cast_float: name = "float"; break;
                default: assert(false);
            }
            _ret = ssnprintf("%s(%s)", name, str.c_str());
        }
        
        virtual void visit(ComponentMask* mask) override {
            auto expr = accept(mask->expr());
            auto strMask = print_dest_mask(mask->mask());
            _ret = ssnprintf("%s%s", expr.c_str(), strMask.c_str());
        }
        
        virtual void visit(IfStatement* invocation) override {
            auto cond = accept(invocation->condition());
            std::string res = "if (" + cond + ") {\n";
            for (auto& st : invocation->statements()) {
                res += "    " + accept(st) + "\n";
            }
            res += "}";
            _ret = res;
        }

    public:
        std::string result() {
            return _ret;
        }
    };
    
    enum class TypeClass {
        integer, fp, boolean
    };
    
    struct TypeInfo {
        TypeClass typeClass;
        int rank;
        ExprType type;
    };
    
    TypeInfo types[] = {
        { TypeClass::integer, 1, ExprType::int32 },
        { TypeClass::integer, 2, ExprType::ivec2 },
        { TypeClass::integer, 3, ExprType::ivec3 },
        { TypeClass::integer, 4, ExprType::ivec4 },
        { TypeClass::boolean, 1, ExprType::boolean },
        { TypeClass::boolean, 2, ExprType::bvec2 },
        { TypeClass::boolean, 3, ExprType::bvec3 },
        { TypeClass::boolean, 4, ExprType::bvec4 },
        { TypeClass::fp,      1, ExprType::fp32 },
        { TypeClass::fp,      2, ExprType::vec2 },
        { TypeClass::fp,      3, ExprType::vec3 },
        { TypeClass::fp,      4, ExprType::vec4 },
    };
    
    class TypeVisitor : public IExpressionVisitor {
        ExprType _ret;
        
        ExprType accept(Expression* expr) {
            expr->accept(this);
            return _ret;
        }
        
        virtual void visit(FloatLiteral* literal) override {
            _ret = ExprType::fp32;
        }
        
        virtual void visit(BinaryOperator* op) override {
            auto leftType = accept(op->args().at(0));
            auto rightType = accept(op->args().at(1));
            if (leftType != rightType || 
                leftType == ExprType::notype ||
                rightType == ExprType::notype)
            {
                _ret = ExprType::notype;
                return;
            }
            _ret = leftType;
        }
        
        virtual void visit(UnaryOperator* op) override {
            _ret = accept(op->args().at(0));
        }
        
        virtual void visit(Swizzle* swizzle) override {
            _ret = accept(swizzle->expr());
        }
        
        virtual void visit(Assignment* assignment) override {
            _ret = ExprType::notype;
        }
        
        virtual void visit(AttributeReference* ref) override {
            _ret = ExprType::vec4;
        }
        
        virtual void visit(RegisterReference* ref) override {
            _ret = ExprType::vec4;
        }
        
        virtual void visit(Invocation* invocation) override {
            auto name = invocation->name();
            auto fi = std::find_if(std::begin(functionInfos), std::end(functionInfos), [=](auto i) {
                return i.name == invocation->name();
            });
            assert(fi != std::end(functionInfos));
            if (name == FunctionName::floor || 
                name == FunctionName::fract ||
                name == FunctionName::abs)
            {
                _ret = accept(invocation->args().at(0));
                return;
            }
            _ret = fi->types[0];
        }
        
        virtual void visit(ComponentMask* mask) override {
            auto exprType = accept(mask->expr());
            auto t = std::find_if(std::begin(types), std::end(types), [=](auto i) {
                return i.type == exprType;
            });
            assert(t != std::end(types));
            auto val = mask->mask().val;
            int rank = val[0] + val[1] + val[2] + val[3];
            auto newType = std::find_if(std::begin(types), std::end(types), [=](auto i) {
                return i.typeClass == t->typeClass && i.rank == rank;
            });
            assert(newType != std::end(types));
            _ret = newType->type;
        }
        
        virtual void visit(IfStatement* mask) override {
            _ret = ExprType::notype;
        }
        
    public:
        ExprType result() {
            return _ret;
        }
    };
    
    class DefaultVisitor : public IExpressionVisitor {
    public:
        virtual void visit(FloatLiteral* literal) override { }
        
        virtual void visit(BinaryOperator* op) override { 
            op->args().at(0)->accept(this);
            op->args().at(1)->accept(this);
        }
        
        virtual void visit(UnaryOperator* op) override {
            op->args().at(0)->accept(this);
        }
        
        virtual void visit(Swizzle* swizzle) override {
            swizzle->expr()->accept(this);
        }
        
        virtual void visit(AttributeReference* ref) override { }
        virtual void visit(RegisterReference* ref) override { }
        
        virtual void visit(ComponentMask* mask) override {
            mask->expr()->accept(this);
        }
        
        virtual void visit(IfStatement* ifst) override { 
            ifst->condition()->accept(this);
            for (auto& st : ifst->statements()) {
                st->accept(this);
            }
        }
        
        virtual void visit(Assignment* assignment) override {
            assignment->dest()->accept(this);
            assignment->expr()->accept(this);
        }
        
        virtual void visit(Invocation* invocation) override {
            for (auto& arg : invocation->args()) {
                arg->accept(this);
            }
        }
    };
    
    class TypeFixerVisitor : public DefaultVisitor {
        TypeVisitor typeVisitor;
        
        ExprType getType(Expression* st) {
            st->accept(&typeVisitor);
            return typeVisitor.result();
        }
        
        Expression* adjustType(Expression* expr, ExprType expected) {
            auto current = getType(expr);
            if (current == expected || expected == ExprType::notype)
                return expr;
            if (expected == ExprType::fp32 && current == ExprType::boolean) {
                auto cast = new Invocation(FunctionName::cast_float, { expr });
                return cast;
            }
            if (expected == ExprType::vec2 && current == ExprType::vec4) {
                auto mask = new ComponentMask(expr, { 1, 1, 0, 0 });
                return mask;
            }
            if (expected == ExprType::vec4 && current == ExprType::fp32) {
                auto vec = new Invocation(FunctionName::vec4, { expr });
                auto swizzle = new Swizzle(vec, {
                    swizzle2bit_t::X,
                    swizzle2bit_t::X,
                    swizzle2bit_t::X,
                    swizzle2bit_t::X
                });
                return swizzle;
            }
            assert(false);
            return expr;
        }
        
        virtual void visit(Assignment* assignment) override {
            DefaultVisitor::visit(assignment);
            auto destType = getType(assignment->dest());
            auto rhsType = getType(assignment->expr());
            if (destType == rhsType)
                return;
            auto adjustedExpr = adjustType(assignment->expr(), destType);
            assignment->releaseAndReplaceExpr(adjustedExpr);
        }
        
        virtual void visit(Invocation* invocation) override {
            DefaultVisitor::visit(invocation);
            auto rti = std::find_if(std::begin(functionInfos), std::end(functionInfos), [=](auto i) {
                return i.name == invocation->name();
            });
            assert(rti != std::end(functionInfos));
            auto args = invocation->args();
            for (auto i = 0u; i < args.size(); ++i) {
                auto& arg = args.at(i);
                auto expectedType = rti->types[i + 1];
                auto adjustedExpr = adjustType(arg, expectedType);
                invocation->releaseAndReplaceArg(i, adjustedExpr);
            }
        }
    };
    
    class InfoCollectorVisitor : public DefaultVisitor {
        int _lastRegisterUsed = -1;
        virtual void visit(RegisterReference* ref) override {
            if (!ref->isC()) {
                _lastRegisterUsed = std::max(_lastRegisterUsed, ref->num());
            }
        }
    public:
        int lastRegisterUsed() {
            return _lastRegisterUsed;
        }
    };
    
    std::string PrintStatement(Statement* stat) {
        PrintVisitor visitor;
        stat->accept(&visitor);
        return visitor.result();
    }

    bool RegisterReference::isC() {
        return _isC;
    }
    
    ComponentMask::ComponentMask(Expression* expr, dest_mask_t mask)
        : _expr(expr), _mask(mask) { }
    
    Expression* ComponentMask::expr() {
        return _expr.get();
    }
    
    dest_mask_t ComponentMask::mask() {
        return _mask;
    }
    
    TernaryOperator::TernaryOperator(std::unique_ptr<Expression> cond, 
                                     std::unique_ptr<Expression> trueBranch,
                                     std::unique_ptr<Expression> falseBranch)
        : Invocation(FunctionName::ternary, { }),
          _cond(std::move(cond)), 
          _trueBranch(std::move(trueBranch)),
          _falseBranch(std::move(falseBranch)) { }

    IfStatement::IfStatement(std::unique_ptr<Expression> expr,
                             std::vector<std::unique_ptr<Statement>> statements)
        : _expr(std::move(expr)), _statements(std::move(statements)) { }
        
    std::vector<Statement*> IfStatement::statements() {
        std::vector<Statement*> res;
        for (auto& st : _statements)
            res.push_back(st.get());
        return res;
    }
    
    Expression* IfStatement::condition() {
        return _expr.get();
    }
    
    std::unique_ptr<Expression> ConvertArgument(FragmentInstr const& i, int n) {
        assert(n < i.opcode.op_count);
        auto& arg = i.arguments[n];
        assert(arg.is_abs + arg.is_neg <= 1);
        Expression* expr = nullptr;
        bool ignoreSwizzle = false;
        if (arg.type == op_type_t::Const) {
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
        if (arg.type == op_type_t::Attr) {
            expr = new AttributeReference(i.intput_attr, i.persp_corr);
        }
        if (arg.type == op_type_t::Temp) {
            expr = new RegisterReference(arg.reg_type, false, arg.reg_num);
        }
        if (arg.is_abs) {
            expr = new Invocation(FunctionName::abs, { expr });
        }
        if (arg.is_neg) {
            expr = new UnaryOperator(FunctionName::neg, expr);
        }
        if (!ignoreSwizzle) {
            if (arg.swizzle.comp[0] != swizzle2bit_t::X ||
                arg.swizzle.comp[1] != swizzle2bit_t::Y ||
                arg.swizzle.comp[2] != swizzle2bit_t::Z ||
                arg.swizzle.comp[3] != swizzle2bit_t::W)
            {
                expr = new Swizzle(expr, arg.swizzle);
            }
        }
        
        return std::unique_ptr<Expression>(expr);
    }
    
    std::vector<std::unique_ptr<Statement>> MakeStatement(FragmentInstr const& i) {
        if (std::find(begin(nops), end(nops), i.opcode.instr) != end(nops)) {
            return { };
        }
        Expression* rhs = nullptr;
        auto it = operators.find(i.opcode.instr);
        if (it != end(operators)) {
            rhs = new BinaryOperator(
                it->second,
                ConvertArgument(i, 0),
                ConvertArgument(i, 1)
            );
        }
        auto fit = functions.find(i.opcode.instr);
        if (fit != end(functions)) {
            std::vector<Expression*> args;
            for (int a = 0; a < i.opcode.op_count; ++a) {
                args.push_back(ConvertArgument(i, a).release());
            }
            rhs = new Invocation(fit->second, args);
        }
        
        if (i.opcode.instr == fragment_op_t::MOV) {
            rhs = ConvertArgument(i, 0).release();
        }
        
        assert(rhs && "not implemented op");
        
        rhs = new ComponentMask(rhs, i.dest_mask);
        
        int regnum = i.reg_num;
        if (i.is_reg_c)
            regnum = i.control == control_mod_t::C0 ? 0 : 1;
        auto dest = new RegisterReference(i.reg, i.is_reg_c, regnum);
        auto mask = new ComponentMask(dest, i.dest_mask);
        auto assign = new Assignment(mask, rhs);
        
        std::vector<std::unique_ptr<Statement>> res;
        res.emplace_back(assign);
        
        if (i.control != control_mod_t::None && !i.is_reg_c) {
            auto cregnum = i.control == control_mod_t::C0 ? 0 : 1;
            auto cdest = new RegisterReference(reg_type_t::H, true, cregnum);
            auto maskedCdest = new ComponentMask(cdest, i.dest_mask);
            auto rhs = new RegisterReference(i.reg, i.is_reg_c, i.reg_num);
            auto maskedRhs = new ComponentMask(rhs, i.dest_mask);
            auto cassign = new Assignment(maskedCdest, maskedRhs);
            res.emplace_back(cassign);
        }
        
        if (i.condition.relation != cond_t::TR) {
            auto func = relationToFunction(i.condition.relation);
            auto sw = i.condition.swizzle;
            assert(sw.comp[0] == sw.comp[1] && 
                   sw.comp[1] == sw.comp[2] &&
                   sw.comp[2] == sw.comp[3]);
            auto regnum = i.condition.is_C1 ? 1 : 0;
            auto reg = new RegisterReference(reg_type_t::H, true, regnum);
            auto swexpr = new Swizzle(reg, i.condition.swizzle);
            std::unique_ptr<Expression> maskExpr(new ComponentMask(swexpr, { 1, 0, 0, 0 }));
            std::unique_ptr<Expression> expr(
                new BinaryOperator(func, std::move(maskExpr), std::make_unique<FloatLiteral>(0)));
            auto ifstat = new IfStatement(std::move(expr), std::move(res));
            res = std::vector<std::unique_ptr<Statement>>();
            res.emplace_back(ifstat);
        }
        
        TypeFixerVisitor typeFixer;
        for (auto& st : res) {
            st->accept(&typeFixer);
        }
        
        return res;
    }

    void Assignment::releaseAndReplaceExpr(Expression* expr) {
        _expr.release();
        _expr.reset(expr);
    }

    void Invocation::releaseAndReplaceArg(int n, Expression* expr) {
        _args.at(n).release();
        _args.at(n).reset(expr);
    }

    int GetLastRegisterNum(Expression* expr) {
        InfoCollectorVisitor visitor;
        expr->accept(&visitor);
        return visitor.lastRegisterUsed();
    }
}
