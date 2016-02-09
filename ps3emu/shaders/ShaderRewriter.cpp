#include "ShaderRewriter.h"
#include "../utils.h"
#include <map>
#include <vector>
#include <algorithm>

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
        { FunctionName::lessThan, { ExprType::bvec4, ExprType::vec4, ExprType::vec4 } },
        { FunctionName::greaterThan, { ExprType::bvec4, ExprType::vec4, ExprType::vec4 } },
        { FunctionName::lessThanEqual, { ExprType::bvec4, ExprType::vec4, ExprType::vec4 } },
        { FunctionName::vec4, 
            { ExprType::vec4, ExprType::fp32, ExprType::fp32, ExprType::fp32, ExprType::fp32 } },
        { FunctionName::fract, { ExprType::notype, ExprType::notype } },
        { FunctionName::floor, { ExprType::notype, ExprType::notype } },
        { FunctionName::ceil, { ExprType::notype, ExprType::notype } },
        { FunctionName::cast_float, { ExprType::fp32, ExprType::notype } },
        { FunctionName::dot2, { ExprType::fp32, ExprType::vec2, ExprType::vec2 } },
        { FunctionName::dot3, { ExprType::fp32, ExprType::vec3, ExprType::vec3 } },
        { FunctionName::dot4, { ExprType::fp32, ExprType::vec4, ExprType::vec4 } },
        { FunctionName::abs, { ExprType::notype, ExprType::notype } },
        { FunctionName::cos, { ExprType::vec4, ExprType::vec4 } },
        { FunctionName::min, { ExprType::vec4, ExprType::vec4, ExprType::vec4 } },
        { FunctionName::max, { ExprType::vec4, ExprType::vec4, ExprType::vec4 } },
        { FunctionName::exp2, { ExprType::vec4, ExprType::vec4, ExprType::vec4 } },
        { FunctionName::sin, { ExprType::vec4, ExprType::vec4 } },
        { FunctionName::lg2, { ExprType::vec4, ExprType::vec4 } },
        { FunctionName::pow, { ExprType::notype, ExprType::notype } },
        { FunctionName::inversesqrt, { ExprType::vec4, ExprType::vec4 } },
        { FunctionName::reverse4f, { ExprType::vec4, ExprType::vec4 } },
        { FunctionName::reverse3f, { ExprType::vec4, ExprType::vec4 } },
        { FunctionName::txl0, { ExprType::vec4, ExprType::notype, ExprType::vec4 } },
        { FunctionName::txl1, { ExprType::vec4, ExprType::notype, ExprType::vec4 } },
        { FunctionName::txl2, { ExprType::vec4, ExprType::notype, ExprType::vec4 } },
        { FunctionName::txl3, { ExprType::vec4, ExprType::notype, ExprType::vec4 } },
        { FunctionName::ftex, { ExprType::vec4, ExprType::int32, ExprType::vec4 } },
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
    
    class PrintVisitor : public IExpressionVisitor {
        std::string _ret;
        
        std::string printOperatorName(FunctionName name) {
            switch (name) {
                case FunctionName::mul: return "*";
                case FunctionName::div: return "/";
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
            if (std::isnan(literal->value()))
                throw std::runtime_error("FloatLiteral should never be nan");
            _ret = ssnprintf("%g", literal->value());
        }
        
        virtual void visit(IntegerLiteral* literal) override {
            _ret = ssnprintf("%d", literal->value());
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
        
        virtual void visit(Variable* var) override {
            if (!var->index()) {
                _ret = var->name();
                return;
            }
            auto index = accept(var->index());
            _ret = ssnprintf("%s[%s]", var->name(), index);
        }
        
        virtual void visit(Invocation* invocation) override {
            auto args = invocation->args();
            
            if (invocation->name() == FunctionName::ftex) {
                assert(invocation->args().size() == 2);
                _ret = ssnprintf("tex%s(%s)", accept(args[0]), accept(args[1]));
                return;
            }
            
            std::string str;
            for (auto i = 0u; i < args.size(); ++i) {
                if (i > 0)
                    str += ", ";
                auto arg = accept(args.at(i));
                str += arg;
            }
            const char* name = nullptr;
            switch (invocation->name()) {
                case FunctionName::vec4: name = "vec4"; break;
                case FunctionName::vec3: name = "vec3"; break;
                case FunctionName::vec2: name = "vec2"; break;
                case FunctionName::fract: name = "fract"; break;
                case FunctionName::floor: name = "floor"; break;
                case FunctionName::ceil: name = "ceil"; break;
                case FunctionName::dot2:
                case FunctionName::dot3:
                case FunctionName::dot4: name = "dot"; break;
                case FunctionName::max: name = "max"; break;
                case FunctionName::min: name = "min"; break;
                case FunctionName::lessThan: name = "lessThan"; break;
                case FunctionName::greaterThan: name = "greaterThan"; break;
                case FunctionName::lessThanEqual: name = "lessThanEqual"; break;
                case FunctionName::abs: name = "abs"; break;
                case FunctionName::cast_float: name = "float"; break;
                case FunctionName::cos: name = "cos"; break;
                case FunctionName::inversesqrt: name = "inversesqrt"; break;
                case FunctionName::sin: name = "sin"; break;
                case FunctionName::reverse4f: name = "reverse4f"; break;
                case FunctionName::reverse3f: name = "reverse3f"; break;
                case FunctionName::txl0: name = "txl0"; break;
                case FunctionName::txl1: name = "txl1"; break;
                case FunctionName::txl2: name = "txl2"; break;
                case FunctionName::txl3: name = "txl3"; break;
                case FunctionName::lg2: name = "log2"; break;
                case FunctionName::pow: name = "pow"; break;
                case FunctionName::exp2: name = "exp2"; break;
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
        
        virtual void visit(IntegerLiteral* literal) override {
            _ret = ExprType::int32;
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
        
        virtual void visit(Variable* ref) override {
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
            if (exprType == ExprType::notype) {
                _ret = ExprType::notype;
                return;
            }
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
        
        virtual void visit(Variable* ref) override { }
        virtual void visit(IntegerLiteral* ref) override { }
        
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
            if (expected == ExprType::fp32 && current == ExprType::vec4) {
                auto mask = new ComponentMask(expr, { 1, 0, 0, 0 });
                return mask;
            }
            if ((expected == ExprType::vec4 && current == ExprType::bvec4) ||
                (expected == ExprType::vec3 && current == ExprType::bvec3) ||
                (expected == ExprType::vec2 && current == ExprType::bvec2)) {
                auto invoke = new Invocation(FunctionName::vec2, { expr });
                return invoke;
            }
            if (expected == ExprType::vec3 && current == ExprType::vec4) {
                auto mask = new ComponentMask(expr, { 1, 1, 1, 0 });
                return mask;
            }
            if (current == ExprType::notype)
                return expr;
            assert(false);
            return expr;
        }
        
        virtual void visit(ComponentMask* mask) override {
            mask->expr()->accept(this);
            if (getType(mask->expr()) == ExprType::fp32)
                mask->mask() = { 1, 0, 0, 0 };
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
        virtual void visit(Variable* ref) override {
            auto num = dynamic_cast<IntegerLiteral*>(ref->index());
            if (ref->name() == "r" && num) {
                _lastRegisterUsed = std::max(_lastRegisterUsed, num->value());
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
    
    Expression* ConvertArgument(FragmentInstr const& i, int n, unsigned constIndex) {
        assert(n < i.opcode.op_count);
        auto& arg = i.arguments[n];
        assert(arg.is_abs + arg.is_neg <= 1);
        Expression* expr = nullptr;
        bool ignoreSwizzle = false;
        if (i.opcode.tex && n == i.opcode.op_count - 1) {
            expr = new IntegerLiteral(i.tex_num);
        } else if (arg.type == op_type_t::Const) {
            expr = new Variable("fconst.c", new IntegerLiteral(constIndex));
        } else if (arg.type == op_type_t::Attr) {
            // the g[] correction has no effect
            auto name = ssnprintf("f_%s", print_attr(i.input_attr));
            expr = new Variable(name, nullptr);
        } else if (arg.type == op_type_t::Temp) {
            auto name = arg.reg_type == reg_type_t::H ? "h" : "r";
            auto index = new IntegerLiteral(arg.reg_num);
            expr = new Variable(name, index);
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
        
        return expr;
    }
    
    void appendCondition(condition_t cond, std::vector<std::unique_ptr<Statement>>& res) {
        if (cond.relation == cond_t::TR)
            return;
        auto func = relationToFunction(cond.relation);
//         auto sw = cond.swizzle;
//         assert(sw.comp[0] == sw.comp[1] && 
//                sw.comp[1] == sw.comp[2] &&
//                sw.comp[2] == sw.comp[3]);
        auto regnum = cond.is_C1 ? 1 : 0;
        auto reg = new Variable("c", new IntegerLiteral(regnum));
        auto swexpr = new Swizzle(reg, cond.swizzle);
        auto maskExpr = new ComponentMask(swexpr, { 1, 0, 0, 0 });
        auto expr = new BinaryOperator(func, maskExpr, new FloatLiteral(0));
        std::vector<Statement*> sts;
        for (auto& st : res)
            sts.push_back(st.release());
        auto ifstat = new IfStatement(expr, sts);
        res.clear();
        res.emplace_back(ifstat);
    }
    
    void appendCAssignment(control_mod_t mod, bool isRegC, dest_mask_t mask,
                           const char* regname, int regnum,
                           std::vector<std::unique_ptr<Statement>>& res)
    {
        if (mod != control_mod_t::None && !isRegC) {
            auto cregnum = mod == control_mod_t::C0 ? 0 : 1;
            auto cdest = new Variable("c", new IntegerLiteral(cregnum));
            auto maskedCdest = new ComponentMask(cdest, mask);
            auto rhs = new Variable(regname, new IntegerLiteral(regnum));
            auto maskedRhs = new ComponentMask(rhs, mask);
            auto cassign = new Assignment(maskedCdest, maskedRhs);
            res.emplace_back(cassign);
        }
    }
    
    std::vector<std::unique_ptr<Statement>> MakeStatement(FragmentInstr const& i, unsigned constIndex) {
        Expression* rhs = nullptr;
        Expression* args[4];
        for (int n = 0; n < i.opcode.op_count; ++n) {
            args[n] = ConvertArgument(i, n, constIndex);
        }
        switch (i.opcode.instr) {
            case fragment_op_t::NOP:
            case fragment_op_t::FENCB:
            case fragment_op_t::FENCT:
                return { };
            case fragment_op_t::ADD: {
                rhs = new BinaryOperator(FunctionName::add, args[0], args[1]);
                break;
            }
            case fragment_op_t::COS: {
                rhs = new Invocation(FunctionName::cos, { args[0] });
                break;
            }
            case fragment_op_t::DP2: {
                rhs = new Invocation(FunctionName::dot2, { args[0], args[1] });
                break;
            }
            case fragment_op_t::DP3: {
                rhs = new Invocation(FunctionName::dot3, { args[0], args[1] });
                break;
            }
            case fragment_op_t::DP4: {
                rhs = new Invocation(FunctionName::dot4, { args[0], args[1] });
                break;
            }
            case fragment_op_t::DIV: {
                rhs = new BinaryOperator(FunctionName::div, args[0], args[1]);
                break;
            }
            case fragment_op_t::DIVSQ: {
                auto mask = new ComponentMask(args[1], { 1, 0, 0, 0 });
                auto inv = new Invocation(FunctionName::inversesqrt, { mask });
                rhs = new BinaryOperator(FunctionName::mul, args[0], inv);
                break;
            }
            case fragment_op_t::EX2: {
                rhs = new Invocation(FunctionName::exp2, { args[0] });
                break;
            }
            case fragment_op_t::FLR: {
                rhs = new Invocation(FunctionName::floor, { args[0] });
                break;
            }
            case fragment_op_t::FRC: {
                rhs = new Invocation(FunctionName::fract, { args[0] });
                break;
            }
            case fragment_op_t::LG2: {
                rhs = new Invocation(FunctionName::lg2, { args[0] });
                break;
            }
            case fragment_op_t::MAD: {
                auto mul = new BinaryOperator(FunctionName::mul, args[0], args[1]);
                auto add = new BinaryOperator(FunctionName::add, mul, args[2]);
                rhs = add;
                break;
            }
            case fragment_op_t::MAX: {
                rhs = new Invocation(FunctionName::max, { args[0], args[1] });
                break;
            }
            case fragment_op_t::MIN: {
                rhs = new Invocation(FunctionName::min, { args[0], args[1] });
                break;
            }
            case fragment_op_t::MOV: {
                rhs = args[0];
                break;
            }
            case fragment_op_t::MUL: {
                rhs = new BinaryOperator(FunctionName::mul, args[0], args[1]);
                break;
            }
            case fragment_op_t::RCP: {
                rhs = new Invocation(FunctionName::pow, { args[0], new FloatLiteral(-1) });
                break;
            }
            case fragment_op_t::RSQ: {
                rhs = new Invocation(FunctionName::inversesqrt, { args[0] });
                break;
            }
            case fragment_op_t::SEQ: {
                rhs = new Invocation(FunctionName::equal, { args[0], args[1] });
                break;
            }
            case fragment_op_t::SFL: {
                rhs = new Invocation(FunctionName::vec4, { 
                    new FloatLiteral(0), new FloatLiteral(0),
                    new FloatLiteral(0), new FloatLiteral(0)
                });
                break;
            }
            case fragment_op_t::SGE: {
                rhs = new Invocation(FunctionName::greaterThanEqual, { args[0], args[1] });
                break;
            }
            case fragment_op_t::SGT: {
                rhs = new Invocation(FunctionName::greaterThan, { args[0], args[1] });
                break;
            }
            case fragment_op_t::SIN: {
                rhs = new Invocation(FunctionName::sin, { args[0], args[1] });
                break;
            }
            case fragment_op_t::SLE: {
                rhs = new Invocation(FunctionName::lessThanEqual, { args[0], args[1] });
                break;
            }
            case fragment_op_t::SLT: {
                rhs = new Invocation(FunctionName::lessThan, { args[0], args[1] });
                break;
            }
            case fragment_op_t::SNE: {
                rhs = new Invocation(FunctionName::notEqual, { args[0], args[1] });
                break;
            }
            case fragment_op_t::STR: {
                rhs = new Invocation(FunctionName::vec4, { 
                    new FloatLiteral(1), new FloatLiteral(1),
                    new FloatLiteral(1), new FloatLiteral(1)
                });
                break;
            }
            case fragment_op_t::TEX: {
                // TODO: txp
                assert(dynamic_cast<IntegerLiteral*>(args[1]));
                rhs = new Invocation(FunctionName::ftex, { args[1], args[0] });
                break;
            }
            default: assert(false);
        }
        
        rhs = new ComponentMask(rhs, i.dest_mask);
        
        int regnum;
        const char* regname;
        if (i.is_reg_c) {
            regnum = i.control == control_mod_t::C0 ? 0 : 1;
            regname = "c";
        } else {
            regnum = i.reg_num;
            regname = i.reg == reg_type_t::H ? "h" : "r";
        }
        auto dest = new Variable(regname, new IntegerLiteral(regnum));
        auto mask = new ComponentMask(dest, i.dest_mask);
        auto assign = new Assignment(mask, rhs);
        
        std::vector<std::unique_ptr<Statement>> res;
        res.emplace_back(assign);
        
        appendCAssignment(i.control, i.is_reg_c, i.dest_mask, regname, regnum, res);
        appendCondition(i.condition, res);
        
        TypeFixerVisitor typeFixer;
        for (auto& st : res) {
            st->accept(&typeFixer);
        }
        
        return res;
    }

    int GetLastRegisterNum(Expression* expr) {
        InfoCollectorVisitor visitor;
        expr->accept(&visitor);
        return visitor.lastRegisterUsed();
    }

    class VertexRefVisitor : public boost::static_visitor<Expression*> {
    public:
        Expression* operator()(vertex_arg_address_ref x) const {
            //assert(false);
            return new IntegerLiteral(0);
        }
        
        Expression* operator()(int x) const {
            return new IntegerLiteral(x);
        }
    };
    
    class VertexArgVisitor : public boost::static_visitor<Expression*> {
    public:
        Expression* operator()(vertex_arg_output_ref_t x) const {
            auto ref = apply_visitor(VertexRefVisitor(), x.ref);
            auto refExpr = new Variable("v_out", ref);
            return new ComponentMask(refExpr,
                { bool(x.mask & 8), bool(x.mask & 4), bool(x.mask & 2), bool(x.mask & 1) }
            );
        }
        
        Expression* convert(std::string name, vertex_arg_ref_t ref, bool neg, bool abs, swizzle_t sw) const {
            auto refNum = apply_visitor(VertexRefVisitor(), ref);
            Expression* expr = new Variable(name, refNum);
            if (abs)
                expr = new UnaryOperator(FunctionName::abs, expr);
            if (neg)
                expr = new UnaryOperator(FunctionName::neg, expr);
            if (!sw.is_xyzw())
                expr = new Swizzle(expr, sw);
            return expr;
        }
        
        Expression* operator()(vertex_arg_input_ref_t x) const {
            return convert("v_in", x.ref, x.is_neg, x.is_abs, x.swizzle);
        }
        
        Expression* operator()(vertex_arg_const_ref_t x) const {
            return convert("constants.c", x.ref, x.is_neg, x.is_abs, x.swizzle);
        }
        
        Expression* operator()(vertex_arg_temp_reg_ref_t x) const {
            auto expr = convert("r", x.ref, x.is_neg, x.is_abs, x.swizzle);
            if (x.mask != 0xf) {
                expr = new ComponentMask(expr, 
                    { bool(x.mask & 8), bool(x.mask & 4), bool(x.mask & 2), bool(x.mask & 1) });
            }
            return expr;
        }
        
        Expression* operator()(vertex_arg_address_reg_ref_t x) const {
            assert(false);
            return nullptr;
        }
        
        Expression* operator()(vertex_arg_cond_reg_ref_t x) const {
            auto mask = dest_mask_t { bool(x.mask & 8), bool(x.mask & 4), bool(x.mask & 2), bool(x.mask & 1) };
            auto index = new IntegerLiteral(x.c);
            return new ComponentMask(new Variable("c", index), mask);
        }
        
        Expression* operator()(vertex_arg_label_ref_t x) const {
            assert(false);
            return nullptr;
        }
        
        Expression* operator()(vertex_arg_tex_ref_t x) const {
            return new IntegerLiteral(x.tex);
        }
    };
    
    std::vector<std::unique_ptr<Statement>> MakeStatement(const VertexInstr& i, unsigned address) {
        Expression* args[4];
        VertexArgVisitor argVisitor;
        for (int j = 0; j < i.op_count; ++j) {
            args[j] = apply_visitor(argVisitor, i.args[j]);
        }
        std::vector<std::unique_ptr<Statement>> vec;
        Expression* rhs, *lhs = nullptr;
        
        switch (i.operation) {
            case vertex_op_t::ADD: {
                rhs = new BinaryOperator(FunctionName::add, args[1], args[2]);
                break;
            }
            case vertex_op_t::MAD: {
                auto mul = new BinaryOperator(FunctionName::mul, args[1], args[2]);
                auto add = new BinaryOperator(FunctionName::add, mul, args[3]);
                rhs = add;
                break;
            }
            case vertex_op_t::ARL: {
                rhs = new Invocation(FunctionName::floor, { args[1] });
                break;
            }
            case vertex_op_t::ARR: {
                rhs = new Invocation(FunctionName::ceil, { args[1] });
                break;
            }
            case vertex_op_t::BRB:
            case vertex_op_t::BRI: {
                lhs = new Variable("void_var", nullptr);
                rhs = new Invocation(FunctionName::branch, { args[0] });
                break;
            }
            case vertex_op_t::CLI:
            case vertex_op_t::CLB: {
                lhs = new Variable("void_var", nullptr);
                rhs = new Invocation(FunctionName::call, { args[0] });
                break;
            }
            case vertex_op_t::COS: {
                rhs = new Invocation(FunctionName::cos, { args[1] });
                break;
            }
            case vertex_op_t::DP3: {
                rhs = new Invocation(FunctionName::dot3, { args[1], args[2] });
                break;
            }
            case vertex_op_t::DP4: {
                rhs = new Invocation(FunctionName::dot4, { args[1], args[2] });
                break;
            }
            case vertex_op_t::DST: {
                auto c0 = new FloatLiteral(1);
                auto c1 = new BinaryOperator(
                    FunctionName::mul,
                    new ComponentMask(args[1], { 0, 1, 0, 0 }),
                    new ComponentMask(args[2], { 0, 1, 0, 0 })
                );
                auto c2 = new ComponentMask(args[1], { 0, 0, 1, 0 });
                auto c3 = new ComponentMask(args[2], { 0, 0, 0, 1 });
                rhs = new Invocation(FunctionName::vec4, { c0, c1, c2, c3 });
                break;
            }
            case vertex_op_t::EXP:
            case vertex_op_t::EX2: {
                rhs = new Invocation(FunctionName::exp2, { args[1] });
                break;
            }
            case vertex_op_t::FLR: {
                rhs = new Invocation(FunctionName::floor, { args[1] });
                break;
            }
            case vertex_op_t::FRC: {
                rhs = new Invocation(FunctionName::fract, { args[1] });
                break;
            }
            case vertex_op_t::LG2:
            case vertex_op_t::LOG: {
                rhs = new Invocation(FunctionName::lg2, { args[1] });
                break;
            }
            case vertex_op_t::LIT: {
                auto c0 = new FloatLiteral(1);
                auto c1 = new ComponentMask(args[1], { 1, 0, 0, 0 });
                auto arg1Clone1 = apply_visitor(argVisitor, i.args[1]);
                auto arg1Clone2 = apply_visitor(argVisitor, i.args[1]);
                auto arg1Clone3 = apply_visitor(argVisitor, i.args[1]);
                auto mul = new BinaryOperator(
                    FunctionName::mul,
                    new ComponentMask(arg1Clone2, { 0, 0, 0, 1 }),
                    new Invocation(
                        FunctionName::lg2,
                        { new ComponentMask(arg1Clone3, { 0, 1, 0, 0 }) }
                    )
                );
                auto c2 = new TernaryOperator(
                    new BinaryOperator(
                        FunctionName::gt,
                        new ComponentMask(arg1Clone1, { 1, 0, 0, 0 }),
                        new FloatLiteral(0)
                    ),
                    new Invocation(FunctionName::exp2, { mul }),
                    new FloatLiteral(0)
                );
                auto c3 = new FloatLiteral(1);
                rhs = new Invocation(FunctionName::vec4, { c0, c1, c2, c3 });
                break;
            }
            case vertex_op_t::MIN: {
                rhs = new Invocation(FunctionName::min, { args[1], args[2] });
                break;
            }
            case vertex_op_t::MAX: {
                rhs = new Invocation(FunctionName::max, { args[1], args[2] });
                break;
            }
            case vertex_op_t::MOV: {
                rhs = args[1];
                break;
            }
            case vertex_op_t::MUL: {
                rhs = new BinaryOperator(FunctionName::mul, args[1], args[2]);
                break;
            }
            case vertex_op_t::MVA: {
                auto arg1Clone = apply_visitor(argVisitor, i.args[1]);
                auto x = new Swizzle(args[1], { swizzle2bit_t::X, swizzle2bit_t::Y });
                auto y = new Swizzle(arg1Clone, { swizzle2bit_t::Z, swizzle2bit_t::W });
                auto sum = new BinaryOperator(FunctionName::add, x, y);
                auto sw = new Swizzle(sum, {
                    swizzle2bit_t::X, swizzle2bit_t::Y, swizzle2bit_t::X, swizzle2bit_t::Y 
                });
                rhs = new Invocation(FunctionName::clamp4i, { sw });
                break;
            }
            case vertex_op_t::RCP: {
                auto mask = new ComponentMask(args[1], { 1, 0, 0, 0 });
                rhs = new Invocation(FunctionName::pow, { mask, new FloatLiteral(-1) });
                break;
            }
            case vertex_op_t::RSQ: {
                rhs = new Invocation(FunctionName::inversesqrt, { args[1] });
                break;
            }
            case vertex_op_t::SEQ: {
                rhs = new Invocation(FunctionName::equal, { args[1], args[2] });
                break;
            }
            case vertex_op_t::SFL: {
                rhs = new Invocation(FunctionName::vec4, { 
                    new FloatLiteral(0), new FloatLiteral(0),
                    new FloatLiteral(0), new FloatLiteral(0)
                });
                break;
            }
            case vertex_op_t::SGE: {
                rhs = new Invocation(FunctionName::greaterThanEqual, { args[1], args[2] });
                break;
            }
            case vertex_op_t::SGT: {
                rhs = new Invocation(FunctionName::greaterThan, { args[1], args[2] });
                break;
            }
            case vertex_op_t::SIN: {
                rhs = new Invocation(FunctionName::sin, { args[1] });
                break;
            }
            case vertex_op_t::SLE: {
                rhs = new Invocation(FunctionName::lessThanEqual, { args[1], args[2] });
                break;
            }
            case vertex_op_t::SLT: {
                rhs = new Invocation(FunctionName::lessThan, { args[1], args[2] });
                break;
            }
            case vertex_op_t::SNE: {
                rhs = new Invocation(FunctionName::notEqual, { args[1], args[2] });
                break;
            }
            case vertex_op_t::SSG: {
                rhs = new Invocation(FunctionName::sign, { args[1] });
                break;
            }
            case vertex_op_t::STR: {
                rhs = new Invocation(FunctionName::vec4, { 
                    new FloatLiteral(1), new FloatLiteral(1),
                    new FloatLiteral(1), new FloatLiteral(1)
                });
                break;
            }
            case vertex_op_t::TXL: {
                auto txl = dynamic_cast<IntegerLiteral*>(args[2]);
                assert(txl);
                auto func = txl->value() == 0 ? FunctionName::txl0
                          : txl->value() == 1 ? FunctionName::txl1
                          : txl->value() == 2 ? FunctionName::txl2
                          : FunctionName::txl3;
                rhs = new Invocation(func, { args[1] });
                break;
            }
            default: assert(false);
        }
        auto mask = dynamic_cast<ComponentMask*>(args[0]);
        if (mask) {
            rhs = new ComponentMask(rhs, mask->mask());
        }
        bool isRegC = i.op_count > 1 && boost::get<vertex_arg_cond_reg_ref_t>(&i.args[0]);
        auto maskVal = mask ? mask->mask() : dest_mask_t { 1, 1, 1, 1 };
        vec.emplace_back(new Assignment(lhs ? lhs : args[0], rhs));
        
        appendCAssignment(i.control, isRegC, maskVal, "abc", 1, vec); // TODO:
        appendCondition(i.condition, vec);
        
        TypeFixerVisitor fixer;
        vec.back()->accept(&fixer);
        return vec;
    }
}
