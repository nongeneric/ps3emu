#include "ShaderRewriter.h"
#include "DefaultVisitor.h"
#include "../utils.h"
#include <map>
#include <set>
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
        { FunctionName::equal, { ExprType::bvec4, ExprType::vec4, ExprType::vec4 } },
        { FunctionName::notEqual, { ExprType::bvec4, ExprType::vec4, ExprType::vec4 } },
        { FunctionName::lessThan, { ExprType::bvec4, ExprType::vec4, ExprType::vec4 } },
        { FunctionName::greaterThan, { ExprType::bvec4, ExprType::vec4, ExprType::vec4 } },
        { FunctionName::greaterThanEqual, { ExprType::bvec4, ExprType::vec4, ExprType::vec4 } },
        { FunctionName::lessThanEqual, { ExprType::bvec4, ExprType::vec4, ExprType::vec4 } },
        { FunctionName::vec4, 
            { ExprType::vec4, ExprType::fp32, ExprType::fp32, ExprType::fp32, ExprType::fp32 } },
        { FunctionName::cast_int, { ExprType::int32, ExprType::fp32 } },
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
        { FunctionName::pow, { ExprType::vec4, ExprType::vec4, ExprType::vec4 } },
        { FunctionName::normalize, { ExprType::vec4, ExprType::vec4 } },
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
            default: assert(false); return FunctionName::none;
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
                case FunctionName::cast_int: name = "int"; break;
                case FunctionName::fract: name = "fract"; break;
                case FunctionName::floor: name = "floor"; break;
                case FunctionName::ceil: name = "ceil"; break;
                case FunctionName::dot2:
                case FunctionName::dot3:
                case FunctionName::dot4: name = "dot"; break;
                case FunctionName::max: name = "max"; break;
                case FunctionName::min: name = "min"; break;
                case FunctionName::equal: name = "equal"; break;
                case FunctionName::notEqual: name = "notEqual"; break;
                case FunctionName::lessThan: name = "lessThan"; break;
                case FunctionName::greaterThan: name = "greaterThan"; break;
                case FunctionName::greaterThanEqual: name = "greaterThanEqual"; break;
                case FunctionName::lessThanEqual: name = "lessThanEqual"; break;
                case FunctionName::abs: name = "abs"; break;
                case FunctionName::cast_float: name = "float"; break;
                case FunctionName::cos: name = "cos"; break;
                case FunctionName::inversesqrt: name = "inversesqrt"; break;
                case FunctionName::sin: name = "sin"; break;
                case FunctionName::normalize: name = "normalize"; break;
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
        
        virtual void visit(SwitchStatement* sw) override {
            std::string cases;
            for (auto& c : sw->cases()) {
                std::string body;
                for (auto& st : c.body) {
                    body += ssnprintf("    %s\n", accept(st.get()));
                }
                cases += ssnprintf("case %d: {\n%s}\n", c.address, body);
            }
            _ret = ssnprintf("switch (%s) {\n%s}", accept(sw->switchOn()), cases);
        }
        
        virtual void visit(BreakStatement* sw) override {
            _ret = "break;";
        }
        
        virtual void visit(WhileStatement* ws) override {
            std::string body;
            for (auto st : ws->body()) {
                body += accept(st) + "\n";
            }
            _ret = ssnprintf("while (%s) {\n%s}", accept(ws->condition()), body);
        }
        
        virtual void visit(TernaryOperator* ternary) override {
            _ret = ssnprintf("(%s ? %s : %s)",
                             accept(ternary->args()[0]),
                             accept(ternary->args()[1]),
                             accept(ternary->args()[2]));
        }

    public:
        std::string& result() {
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
    
    class TypeVisitor : public DefaultVisitor {
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
            if (ref->name() == "nip") {
                _ret = ExprType::int32;
            } else if (ref->name() == "a") {
                _ret = ExprType::ivec4;
            } else {
                _ret = ExprType::vec4;
            }
        }
        
        virtual void visit(TernaryOperator* ternary) override {
            _ret = accept(ternary->args().at(2));   
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
                return new Invocation(FunctionName::cast_float, { expr });
            }
            if (expected == ExprType::vec2 && current == ExprType::vec4) {
                return new ComponentMask(expr, { 1, 1, 0, 0 });
            }
            if (expected == ExprType::vec4 && current == ExprType::fp32) {
                return new Invocation(FunctionName::vec4, { expr });
            }
            if (expected == ExprType::vec3 && current == ExprType::fp32) {
                return new Invocation(FunctionName::vec3, { expr });
            }
            if (expected == ExprType::vec2 && current == ExprType::fp32) {
                return new Invocation(FunctionName::vec2, { expr });
            }
            if (expected == ExprType::fp32 && current == ExprType::vec4) {
                return new ComponentMask(expr, { 1, 0, 0, 0 });
            }
            if (expected == ExprType::vec4 && current == ExprType::bvec4) {
                return new Invocation(FunctionName::vec4, { expr });
            }
            if (expected == ExprType::vec3 && current == ExprType::bvec3) {
                return new Invocation(FunctionName::vec3, { expr });
            }
            if (expected == ExprType::vec2 && current == ExprType::bvec2) {
                return new Invocation(FunctionName::vec2, { expr });
            }
            if (expected == ExprType::vec3 && current == ExprType::vec4) {
                return new ComponentMask(expr, { 1, 1, 1, 0 });
            }
            if (expected == ExprType::int32 && current == ExprType::fp32) {
                return new Invocation(FunctionName::cast_int, { expr });
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
        
        virtual void visit(TernaryOperator* ternary) override {
            auto args = ternary->args();
            args.at(0)->accept(this);
            args.at(1)->accept(this);
            args.at(2)->accept(this);
            auto trueExpr = args[1];
            auto falseType = getType(args[2]);
            auto adjustedTrueExpr = adjustType(trueExpr, falseType);
            ternary->releaseAndReplaceArg(1, adjustedTrueExpr);
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
        
        virtual void visit(BreakStatement* br) override { }
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
                rhs = new Invocation(FunctionName::sin, { args[0] });
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
            case fragment_op_t::NRM: {
                rhs = new Invocation(FunctionName::normalize, { args[0] });
                break;
            }
            default: assert(false);
        }
        
        rhs = new ComponentMask(rhs, i.dest_mask);
        
        if (i.scale != scale_t::None) {
            auto mul = [&] {
                switch (i.scale) {
                    case scale_t::D2: return 1.0f / 2.0f;
                    case scale_t::D4: return 1.0f / 4.0f;
                    case scale_t::D8: return 1.0f / 8.0f;
                    case scale_t::M2: return 2.0f;
                    case scale_t::M4: return 4.0f;
                    case scale_t::M8: return 8.0f;
                    default: throw std::runtime_error("illegal scale");
                }
            }();
            rhs = new BinaryOperator(FunctionName::mul, new FloatLiteral(mul), rhs);
        }
        
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
            Expression* var = new Variable("a", new IntegerLiteral(x.reg));
            var = new ComponentMask(var,
                                    {x.component == 0,
                                     x.component == 1,
                                     x.component == 2,
                                     x.component == 3});
            return new BinaryOperator(
                FunctionName::add, var, new IntegerLiteral(x.displ));
        }
        
        Expression* operator()(int x) const {
            return new IntegerLiteral(x);
        }
    };
    
    class VertexArgVisitor : public boost::static_visitor<Expression*> {
    public:
        Expression* convert(std::string name, vertex_arg_ref_t ref, bool neg, bool abs, swizzle_t sw, int mask) const {
            auto refNum = apply_visitor(VertexRefVisitor(), ref);
            Expression* expr = new Variable(name, refNum);
            if (abs)
                expr = new Invocation(FunctionName::abs, { expr });
            if (neg)
                expr = new UnaryOperator(FunctionName::neg, expr);
            if (!sw.is_xyzw())
                expr = new Swizzle(expr, sw);
            if (mask != 0xf) {
                expr = new ComponentMask(expr, 
                    { bool(mask & 8), bool(mask & 4), bool(mask & 2), bool(mask & 1) });
            }
            return expr;
        }
        
        Expression* operator()(vertex_arg_output_ref_t x) const {
            return convert("v_out", x.ref, false, false, swizzle_xyzw, x.mask);
        }
        
        Expression* operator()(vertex_arg_input_ref_t x) const {
            return convert("v_in", x.ref, x.is_neg, x.is_abs, x.swizzle, 0xf);
        }
        
        Expression* operator()(vertex_arg_const_ref_t x) const {
            return convert("constants.c", x.ref, x.is_neg, x.is_abs, x.swizzle, 0xf);
        }
        
        Expression* operator()(vertex_arg_temp_reg_ref_t x) const {
            return convert("r", x.ref, x.is_neg, x.is_abs, x.swizzle, x.mask);
        }
        
        Expression* operator()(vertex_arg_address_reg_ref_t x) const {
            return convert("a", x.a, false, false, swizzle_xyzw, x.mask);
        }
        
        Expression* operator()(vertex_arg_cond_reg_ref_t x) const {
            return convert("c", x.c, false, false, swizzle_xyzw, x.mask);
        }
        
        Expression* operator()(vertex_arg_label_ref_t x) const {
            return new IntegerLiteral(x.l);
        }
        
        Expression* operator()(vertex_arg_tex_ref_t x) const {
            return new IntegerLiteral(x.tex);
        }
    };
    
    Expression* wrapMask(Expression* lhs, Expression* expr) {
        auto mask = dynamic_cast<ComponentMask*>(lhs);
        if (mask) {
            return new ComponentMask(expr, mask->mask());
        }
        return expr;
    }
    
    std::vector<std::unique_ptr<Statement>> MakeStatement(const VertexInstr& i, unsigned address) {
        VertexArgVisitor argVisitor;
        auto arg = [&](int j) {
            return apply_visitor(argVisitor, i.args[j]);
        };
        
        Expression* rhs = nullptr, *lhs = nullptr;
        
        switch (i.operation) {
            case vertex_op_t::ADD: {
                rhs = new BinaryOperator(FunctionName::add, arg(1), arg(2));
                break;
            }
            case vertex_op_t::MAD: {
                auto mul = new BinaryOperator(FunctionName::mul, arg(1), arg(2));
                auto add = new BinaryOperator(FunctionName::add, mul, arg(3));
                rhs = add;
                break;
            }
            case vertex_op_t::ARL: {
                rhs = new Invocation(FunctionName::floor, { arg(1) });
                break;
            }
            case vertex_op_t::ARR: {
                rhs = new Invocation(FunctionName::ceil, { arg(1) });
                break;
            }
            case vertex_op_t::BRB:
            case vertex_op_t::BRI: {
                lhs = new Variable("nip", nullptr);
                rhs = arg(0);
                break;
            }
            case vertex_op_t::CLI:
            case vertex_op_t::CLB: {
                lhs = new Variable("void_var", nullptr);
                rhs = new Invocation(FunctionName::call, { arg(0) });
                break;
            }
            case vertex_op_t::COS: {
                rhs = new Invocation(FunctionName::cos, { arg(1) });
                break;
            }
            case vertex_op_t::DP3: {
                rhs = new Invocation(FunctionName::dot3, { arg(1), arg(2) });
                break;
            }
            case vertex_op_t::DP4: {
                rhs = new Invocation(FunctionName::dot4, { arg(1), arg(2) });
                break;
            }
            case vertex_op_t::DPH: {
                auto x = new ComponentMask(arg(1), { 1, 0, 0, 0 });
                auto y = new ComponentMask(arg(1), { 0, 1, 0, 0 });
                auto z = new ComponentMask(arg(1), { 0, 0, 1, 0 });
                auto w = new FloatLiteral(1);
                auto xyz1 = new Invocation(FunctionName::vec4, { x, y, z, w });
                rhs = new Invocation(FunctionName::dot4, { xyz1, arg(2) });
                break;
            }
            case vertex_op_t::DST: {
                auto c0 = new FloatLiteral(1);
                auto c1 = new BinaryOperator(
                    FunctionName::mul,
                    new ComponentMask(arg(1), { 0, 1, 0, 0 }),
                    new ComponentMask(arg(2), { 0, 1, 0, 0 })
                );
                auto c2 = new ComponentMask(arg(1), { 0, 0, 1, 0 });
                auto c3 = new ComponentMask(arg(2), { 0, 0, 0, 1 });
                rhs = new Invocation(FunctionName::vec4, { c0, c1, c2, c3 });
                break;
            }
            case vertex_op_t::EXP:
            case vertex_op_t::EX2: {
                rhs = new Invocation(FunctionName::exp2, { arg(1) });
                break;
            }
            case vertex_op_t::FLR: {
                rhs = new Invocation(FunctionName::floor, { arg(1) });
                break;
            }
            case vertex_op_t::FRC: {
                rhs = new Invocation(FunctionName::fract, { arg(1) });
                break;
            }
            case vertex_op_t::LG2:
            case vertex_op_t::LOG: {
                rhs = new Invocation(FunctionName::lg2, { arg(1) });
                break;
            }
            case vertex_op_t::LIT: {
                auto c0 = new FloatLiteral(1);
                auto c1 = new ComponentMask(arg(1), { 1, 0, 0, 0 });
                auto mul = new BinaryOperator(
                    FunctionName::mul,
                    new ComponentMask(arg(1), { 0, 0, 0, 1 }),
                    new Invocation(
                        FunctionName::lg2,
                        { new ComponentMask(arg(1), { 0, 1, 0, 0 }) }
                    )
                );
                auto c2 = new TernaryOperator(
                    new BinaryOperator(
                        FunctionName::gt,
                        new ComponentMask(arg(1), { 1, 0, 0, 0 }),
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
                rhs = new Invocation(FunctionName::min, { arg(1), arg(2) });
                break;
            }
            case vertex_op_t::MAX: {
                rhs = new Invocation(FunctionName::max, { arg(1), arg(2) });
                break;
            }
            case vertex_op_t::MOV: {
                break;
            }
            case vertex_op_t::MUL: {
                rhs = new BinaryOperator(FunctionName::mul, arg(1), arg(2));
                break;
            }
            case vertex_op_t::MVA: {
                auto x = new Swizzle(arg(1), { swizzle2bit_t::X, swizzle2bit_t::Y });
                auto y = new Swizzle(arg(1), { swizzle2bit_t::Z, swizzle2bit_t::W });
                auto sum = new BinaryOperator(FunctionName::add, x, y);
                auto sw = new Swizzle(sum, {
                    swizzle2bit_t::X, swizzle2bit_t::Y, swizzle2bit_t::X, swizzle2bit_t::Y 
                });
                rhs = new Invocation(FunctionName::clamp4i, { sw });
                break;
            }
            case vertex_op_t::RCP: {
                auto mask = new ComponentMask(arg(1), { 1, 0, 0, 0 });
                rhs = new Invocation(FunctionName::pow, { mask, new FloatLiteral(-1) });
                break;
            }
            case vertex_op_t::RSQ: {
                rhs = new Invocation(FunctionName::inversesqrt, { arg(1) });
                break;
            }
            case vertex_op_t::SEQ: {
                rhs = new Invocation(FunctionName::equal, { arg(1), arg(2) });
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
                rhs = new Invocation(FunctionName::greaterThanEqual, { arg(1), arg(2) });
                break;
            }
            case vertex_op_t::SGT: {
                rhs = new Invocation(FunctionName::greaterThan, { arg(1), arg(2) });
                break;
            }
            case vertex_op_t::SIN: {
                rhs = new Invocation(FunctionName::sin, { arg(1) });
                break;
            }
            case vertex_op_t::SLE: {
                rhs = new Invocation(FunctionName::lessThanEqual, { arg(1), arg(2) });
                break;
            }
            case vertex_op_t::SLT: {
                rhs = new Invocation(FunctionName::lessThan, { arg(1), arg(2) });
                break;
            }
            case vertex_op_t::SNE: {
                rhs = new Invocation(FunctionName::notEqual, { arg(1), arg(2) });
                break;
            }
            case vertex_op_t::SSG: {
                rhs = new Invocation(FunctionName::sign, { arg(1) });
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
                auto txl = dynamic_cast<IntegerLiteral*>(arg(2));
                assert(txl);
                auto func = txl->value() == 0 ? FunctionName::txl0
                          : txl->value() == 1 ? FunctionName::txl1
                          : txl->value() == 2 ? FunctionName::txl2
                          : FunctionName::txl3;
                rhs = new Invocation(func, { arg(1) });
                break;
            }
            case vertex_op_t::NOP: {
                lhs = new WhileStatement(new Variable("false", nullptr), {});
                break;
            }
            default: assert(false);
        }
        auto mask = dynamic_cast<ComponentMask*>(arg(0));
        bool isRegC = i.op_count > 1 && boost::get<vertex_arg_cond_reg_ref_t>(&i.args[0]);
        auto maskVal = mask ? mask->mask() : dest_mask_t { 1, 1, 1, 1 };
        
        std::vector<std::unique_ptr<Statement>> vec;
        if (rhs) {
            auto dest = arg(0);
            vec.emplace_back(new Assignment(lhs ? lhs : dest, wrapMask(dest, rhs)));
            if (i.operation == vertex_op_t::BRI) {
                vec.emplace_back(new BreakStatement());
            }
        } else {        
            if (i.operation == vertex_op_t::MOV) {
                if (i.op_count == 2) {
                    auto dest = arg(0);
                    vec.emplace_back(new Assignment(dest, wrapMask(dest, arg(1))));
                } else {
                    assert(i.op_count == 3);
                    auto dest = arg(0);
                    vec.emplace_back(new Assignment(dest, wrapMask(dest, arg(2))));
                    dest = arg(1);
                    vec.emplace_back(new Assignment(dest, wrapMask(dest, arg(2))));
                }
            } else {
                vec.emplace_back(dynamic_cast<Statement*>(lhs));
            }
        }

        appendCAssignment(i.control, isRegC, maskVal, "abc", 1, vec); // TODO:
        appendCondition(i.condition, vec);
        
        TypeFixerVisitor fixer;
        for (auto& assignment : vec) {
            assignment->address(address);
            assignment->accept(&fixer);
        }
        return vec;
    }
    
    class DoesContainBranchVisitor : public DefaultVisitor {
        virtual void visit(Assignment* assignment) override {
            if (auto variable = dynamic_cast<Variable*>(assignment->dest())) {
                if (variable->name() == "nip") {
                    result = true;
                    auto label = dynamic_cast<IntegerLiteral*>(assignment->expr());
                    assert(label);
                    labels.insert(label->value());
                    return;
                }
            }
            DefaultVisitor::visit(assignment);
        }
    public:
        bool result = false;
        std::set<int> labels;
    };
    
    std::vector<std::unique_ptr<Statement>> RewriteBranches(
        std::vector<std::unique_ptr<Statement>> uniqueStatements) {
        DoesContainBranchVisitor visitor;
        for (auto& st : uniqueStatements) {
            st->accept(&visitor);
        }
        if (!visitor.result)
            return uniqueStatements;
        auto sts = release_unique(uniqueStatements);
        auto sw = new SwitchStatement(new Variable("nip", nullptr));
        
        std::vector<Statement*> curCase;
        for (auto i = 0u; i < sts.size(); ++i) {
            curCase.push_back(sts[i]);
            if (i != sts.size() - 1) {
                auto nextAddress = sts[i + 1]->address();
                if (i != sts.size() - 1 && sts[i]->address() == nextAddress)
                    continue;
                if (visitor.labels.find(nextAddress) == end(visitor.labels))
                    continue;
            }
            if (i == sts.size() - 1) {
                curCase.push_back(new Assignment(new Variable("nip", nullptr),
                                                 new IntegerLiteral(-1)));
            }
            sw->addCase(curCase.front()->address(), curCase);
            curCase.clear();
        }
        
        std::vector<Statement*> res = {
            new Assignment(new Variable("nip", nullptr), new IntegerLiteral(0)),
            new WhileStatement(new BinaryOperator(FunctionName::ne,
                                                  new Variable("nip", nullptr),
                                                  new IntegerLiteral(-1)),
                               { sw })
        };
        return pack_unique(res);
    }
    
    void UsedConstsVisitor::visit(Variable* variable) {
        if (variable->name() == "constants.c") {
            if (auto index = dynamic_cast<IntegerLiteral*>(variable->index())) {
                _consts.insert(index->value());
            }
        }
    }
    
    const std::set< unsigned int >& UsedConstsVisitor::consts() {
        return _consts;
    }
}
