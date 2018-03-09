#include "ShaderRewriter.h"
#include "DefaultVisitor.h"
#include "../utils.h"
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <cmath>
#include <boost/range/algorithm.hpp>

namespace ShaderRewriter {
    FunctionName relationToFunction(cond_t relation) {
        switch (relation) {
            case cond_t::EQ:
            case cond_t::FL: return FunctionName::equal;
            case cond_t::GE: return FunctionName::greaterThanEqual;
            case cond_t::GT: return FunctionName::greaterThan;
            case cond_t::LE: return FunctionName::lessThanEqual;
            case cond_t::LT: return FunctionName::lessThan;
            case cond_t::NE: return FunctionName::notEqual;
            default: assert(false); return FunctionName::none;
        }
    }
    
    class PrintVisitor : public IExpressionVisitor {
        std::string _ret;
        unsigned level = 0;
        std::string indent(unsigned level) {
            return std::string(level * 4, ' ');
        }
        
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
        
        virtual void visit(IfStubFragmentStatement* st) override {
            return visit((IfStatement*)st);
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
            _ret = ssnprintf("%s%s = %s;", indent(level), var.c_str(), expr.c_str());
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
            
            if (invocation->name() == FunctionName::ftxp) {
                assert(invocation->args().size() == 2);
                _ret = ssnprintf("tex%sProj(%s)", accept(args[0]), accept(args[1]));
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
                case FunctionName::ivec4: name = "ivec4"; break;
                case FunctionName::cast_int: name = "int"; break;
                case FunctionName::fract: name = "fract"; break;
                case FunctionName::floor: name = "floor"; break;
                case FunctionName::ceil: name = "ceil"; break;
                case FunctionName::dot: name = "dot"; break;
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
                case FunctionName::unpackUnorm4x8: name = "unpackUnorm4x8"; break;
                case FunctionName::floatBitsToUint: name = "floatBitsToUint"; break;
                case FunctionName::clamp: name = "clamp"; break;
                case FunctionName::mix: name = "mix"; break;
                case FunctionName::any: name = "any"; break;
                default: assert(false);
            }
            _ret = ssnprintf("%s(%s)", name, str.c_str());
        }
        
        virtual void visit(ComponentMask* mask) override {
            auto expr = accept(mask->expr());
            auto strMask = print_dest_mask(mask->mask());
            _ret = ssnprintf("%s%s", expr.c_str(), strMask.c_str());
        }
        
        virtual void visit(IfStatement* ifst) override {
            auto cond = accept(ifst->condition());
            std::string res = indent(level) + "if (" + cond + ") {\n";
            level++;
            for (auto& st : ifst->trueBlock()) {
                res += accept(st) + "\n";
            }
            if (!ifst->falseBlock().empty()) {
                res += indent(level - 1) + "} else {\n";
                for (auto& st : ifst->falseBlock()) {
                    res += accept(st) + "\n";
                }
            }
            level--;
            res += indent(level) + "}";
            _ret = res;
        }
        
        virtual void visit(SwitchStatement* sw) override {
            std::string cases;
            for (auto& c : sw->cases()) {
                std::string body;
                for (auto& st : c.body) {
                    body += ssnprintf("    %s\n", accept(st));
                }
                cases += ssnprintf("case %d: {\n%s}\n", c.address, body);
            }
            _ret = ssnprintf("switch (%s) {\n%s}", accept(sw->switchOn()), cases);
        }
        
        virtual void visit(BreakStatement* sw) override {
            _ret = indent(level) + "break;";
        }
        
        virtual void visit(DiscardStatement* sw) override {
            _ret = indent(level) + "discard;";
        }
        
        virtual void visit(WhileStatement* ws) override {
            auto cond = accept(ws->condition());
            if (ws->body().empty()) {
                _ret = ssnprintf("%swhile (%s) { }", indent(level), cond);
                return;
            }
            std::string body;
            for (auto st : ws->body()) {
                body += accept(st) + "\n";
            }
            _ret = ssnprintf("%swhile (%s) {\n%s}", indent(level), cond, body);
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
    
    class InfoCollectorVisitor : public DefaultVisitor {
        int _lastRegisterUsed = -1;
        int _lastHRegisterUsed = -1;
        virtual void visit(Variable* ref) override {
            auto num = dynamic_cast<IntegerLiteral*>(ref->index());
            if (ref->name() == "r" && num) {
                _lastRegisterUsed = std::max(_lastRegisterUsed, num->value());
            }
            if (ref->name() == "h" && num) {
                _lastHRegisterUsed = std::max(_lastHRegisterUsed, num->value());
            }
        }
    public:
        int lastRegisterUsed() {
            return _lastRegisterUsed;
        }
        
        int lastHRegisterUsed() {
            return _lastHRegisterUsed;
        }
    };
    
    Expression* adaptToSingleScalar(ASTContext& context, Expression* expr) {
        return context.make<Swizzle>(expr, swizzle_t{
            swizzle2bit_t::X,
            swizzle2bit_t::X,
            swizzle2bit_t::X,
            swizzle2bit_t::X
        });
    };
    
    std::string PrintStatement(Statement* stat) {
        PrintVisitor visitor;
        stat->accept(&visitor);
        return visitor.result();
    }    
    
    Expression* ConvertArgument(ASTContext& context,
                                FragmentInstr const& i,
                                int n,
                                unsigned constIndex) {
        assert(n < i.opcode.op_count);
        auto& arg = i.arguments[n];
        Expression* expr = nullptr;
        if (i.opcode.tex && n == i.opcode.op_count - 1) {
            expr = context.make<IntegerLiteral>(i.tex_num);
        } else if (arg.type == op_type_t::Const) {
            expr = context.make<Variable>("fconst.c", context.make<IntegerLiteral>(constIndex));
        } else if (arg.type == op_type_t::Attr) {
            // the g[] correction has no effect
            auto name = ssnprintf("f_%s", print_attr(i.input_attr));
            expr = context.make<Variable>(name, nullptr);
        } else if (arg.type == op_type_t::Temp) {
            auto name = arg.reg_type == reg_type_t::H ? "h" : "r";
            auto index = context.make<IntegerLiteral>(arg.reg_num);
            expr = context.make<Variable>(name, index);
        }
        if (arg.is_abs) {
            expr = context.make<Invocation>(FunctionName::abs, expr );
        }
        if (arg.is_neg) {
            expr = context.make<UnaryOperator>(FunctionName::neg, expr);
        }
        if (arg.swizzle.comp[0] != swizzle2bit_t::X ||
            arg.swizzle.comp[1] != swizzle2bit_t::Y ||
            arg.swizzle.comp[2] != swizzle2bit_t::Z ||
            arg.swizzle.comp[3] != swizzle2bit_t::W)
        {
            expr = context.make<Swizzle>(expr, arg.swizzle);
        }
        return expr;
    }
    
    Expression* makeCondition(ASTContext& context, condition_t cond) {
        if (cond.relation == cond_t::TR)
            return nullptr;
        auto c = context.make<Variable>("c", context.make<IntegerLiteral>(cond.is_C1 ? 1 : 0));
        auto swizzled_c = context.make<Swizzle>(c, cond.swizzle);
        auto vec = context.make<Invocation>(FunctionName::vec4, context.make<FloatLiteral>(0));
        auto func = relationToFunction(cond.relation);
        return context.make<Invocation>(func, swizzled_c, vec);
    }
    
    Expression* appendCondition(ASTContext& context,
                                condition_t cond,
                                Expression* expr,
                                Expression* lhs) {
        auto relation = makeCondition(context, cond);
        if (!relation)
            return expr;
        return context.make<Invocation>(FunctionName::mix, lhs, expr, relation);
    }
    
    Assignment* makeCAssignment(ASTContext& context,
                                control_mod_t mod,
                                bool isRegC,
                                Expression* lhs) {
        if (mod != control_mod_t::None && !isRegC) {
            auto cregnum = mod == control_mod_t::C0 ? 0 : 1;
            Expression* clhs = context.make<Variable>("c", context.make<IntegerLiteral>(cregnum));
            auto mask = dynamic_cast<ComponentMask*>(lhs);
            if (mask) {
                clhs = context.make<ComponentMask>(clhs, mask->mask());
            }
            return context.make<Assignment>(clhs, lhs);
        }
        return nullptr;
    }
    
    Expression* clamp(ASTContext& context, Expression* expr, float min, float max) {
        return context.make<Invocation>(FunctionName::clamp,
                              expr, context.make<FloatLiteral>(min), context.make<FloatLiteral>(max));
    }
    
    std::vector<Statement*> MakeStatement(ASTContext& context,
                                          FragmentInstr const& i,
                                          unsigned constIndex) {
        Expression* rhs = nullptr;
        Expression* args[4];
        for (int n = 0; n < i.opcode.op_count; ++n) {
            args[n] = ConvertArgument(context, i, n, constIndex);
        }
        
        auto isScalarRhs = false;
        std::vector<Statement*> res;
        switch (i.opcode.instr) {
            case fragment_op_t::NOP:
            case fragment_op_t::FENCB:
            case fragment_op_t::FENCT:
                res.emplace_back(
                    context.make<WhileStatement>(context.make<Variable>("false", nullptr), StV{}));
                return res;
            case fragment_op_t::ADD: {
                rhs = context.make<BinaryOperator>(FunctionName::add, args[0], args[1]);
                break;
            }
            case fragment_op_t::COS: {
                rhs = context.make<Invocation>(FunctionName::cos, adaptToSingleScalar(context, args[0]));
                break;
            }
            case fragment_op_t::DP2: {
                isScalarRhs = true;
                auto arg0 = context.make<ComponentMask>(args[0], dest_mask_t{1, 1, 0, 0});
                auto arg1 = context.make<ComponentMask>(args[1], dest_mask_t{1, 1, 0, 0});
                rhs = context.make<Invocation>(FunctionName::dot, arg0, arg1);
                break;
            }
            case fragment_op_t::DP3: {
                isScalarRhs = true;
                auto arg0 = context.make<ComponentMask>(args[0], dest_mask_t{1, 1, 1, 0});
                auto arg1 = context.make<ComponentMask>(args[1], dest_mask_t{1, 1, 1, 0});
                rhs = context.make<Invocation>(FunctionName::dot, arg0, arg1);
                break;
            }
            case fragment_op_t::DP4: {
                isScalarRhs = true;
                rhs = context.make<Invocation>(FunctionName::dot, args[0], args[1]);
                break;
            }
            case fragment_op_t::DIV: {
                rhs = context.make<BinaryOperator>(FunctionName::div, args[0], adaptToSingleScalar(context, args[1]));
                break;
            }
            case fragment_op_t::DIVSQ: {
                auto inv = context.make<Invocation>(FunctionName::inversesqrt, adaptToSingleScalar(context, args[1]));
                rhs = context.make<BinaryOperator>(FunctionName::mul, args[0], inv);
                break;
            }
            case fragment_op_t::EX2: {
                rhs = context.make<Invocation>(FunctionName::exp2, adaptToSingleScalar(context, args[0]));
                break;
            }
            case fragment_op_t::FLR: {
                rhs = context.make<Invocation>(FunctionName::floor, args[0]);
                break;
            }
            case fragment_op_t::FRC: {
                rhs = context.make<Invocation>(FunctionName::fract, args[0]);
                break;
            }
            case fragment_op_t::LG2: {
                rhs = context.make<Invocation>(FunctionName::lg2, adaptToSingleScalar(context, args[0]));
                break;
            }
            case fragment_op_t::MAD: {
                auto mul = context.make<BinaryOperator>(FunctionName::mul, args[0], args[1]);
                auto add = context.make<BinaryOperator>(FunctionName::add, mul, args[2]);
                rhs = add;
                break;
            }
            case fragment_op_t::MAX: {
                rhs = context.make<Invocation>(FunctionName::max, args[0], args[1]);
                break;
            }
            case fragment_op_t::MIN: {
                rhs = context.make<Invocation>(FunctionName::min, args[0], args[1]);
                break;
            }
            case fragment_op_t::MOV: {
                rhs = args[0];
                break;
            }
            case fragment_op_t::MUL: {
                rhs = context.make<BinaryOperator>(FunctionName::mul, args[0], args[1]);
                break;
            }
            case fragment_op_t::RCP: {
                auto exp = context.make<Invocation>(FunctionName::vec4, context.make<FloatLiteral>(-1));
                rhs = context.make<Invocation>(FunctionName::pow, adaptToSingleScalar(context, args[0]), exp);
                break;
            }
            case fragment_op_t::RSQ: {
                rhs = context.make<Invocation>(FunctionName::inversesqrt, adaptToSingleScalar(context, args[0]));
                break;
            }
            case fragment_op_t::SEQ: {
                auto func = context.make<Invocation>(FunctionName::equal, args[0], args[1]);
                rhs = context.make<Invocation>(FunctionName::vec4, func);
                break;
            }
            case fragment_op_t::SFL: {
                rhs = context.make<Invocation>(FunctionName::vec4,
                    context.make<FloatLiteral>(0), context.make<FloatLiteral>(0),
                    context.make<FloatLiteral>(0), context.make<FloatLiteral>(0)
                );
                break;
            }
            case fragment_op_t::SGE: {
                auto func = context.make<Invocation>(FunctionName::greaterThanEqual, args[0], args[1]);
                rhs = context.make<Invocation>(FunctionName::vec4, func);
                break;
            }
            case fragment_op_t::SGT: {
                auto func = context.make<Invocation>(FunctionName::greaterThan, args[0], args[1]);
                rhs = context.make<Invocation>(FunctionName::vec4, func);
                break;
            }
            case fragment_op_t::SIN: {
                rhs = context.make<Invocation>(FunctionName::sin, adaptToSingleScalar(context, args[0]));
                break;
            }
            case fragment_op_t::SLE: {
                auto func = context.make<Invocation>(FunctionName::lessThanEqual, args[0], args[1]);
                rhs = context.make<Invocation>(FunctionName::vec4, func);
                break;
            }
            case fragment_op_t::SLT: {
                auto func = context.make<Invocation>(FunctionName::lessThan, args[0], args[1]);
                rhs = context.make<Invocation>(FunctionName::vec4, func);
                break;
            }
            case fragment_op_t::SNE: {
                auto func = context.make<Invocation>(FunctionName::notEqual, args[0], args[1]);
                rhs = context.make<Invocation>(FunctionName::vec4, func);
                break;
            }
            case fragment_op_t::STR: {
                rhs = context.make<Invocation>(FunctionName::vec4,
                    context.make<FloatLiteral>(1), context.make<FloatLiteral>(1),
                    context.make<FloatLiteral>(1), context.make<FloatLiteral>(1)
                );
                break;
            }
            case fragment_op_t::TEX: {
                assert(dynamic_cast<IntegerLiteral*>(args[1]));
                rhs = context.make<Invocation>(FunctionName::ftex, args[1], args[0]);
                break;
            }
            case fragment_op_t::TXP: {
                assert(dynamic_cast<IntegerLiteral*>(args[1]));
                rhs = context.make<Invocation>(FunctionName::ftxp, args[1], args[0]);
                break;
            }
            case fragment_op_t::NRM: {
                rhs = context.make<Invocation>(FunctionName::normalize, args[0]);
                break;
            }
            case fragment_op_t::UPB: {
                rhs = context.make<Invocation>(
                    FunctionName::unpackUnorm4x8,
                    context.make<Invocation>(FunctionName::floatBitsToUint, args[0]));
                break;
            }
            case fragment_op_t::KIL: {
                break;
            }
            case fragment_op_t::IFE: {
                break;
            }
            default: assert(false);
        }
        
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
            rhs = context.make<BinaryOperator>(FunctionName::mul, context.make<FloatLiteral>(mul), rhs);
        }
        
        if (i.is_sat || i.is_bx2) {
            rhs = clamp(context, rhs, 0.0f, 1.0f);
        } else {
            if (i.clamp == clamp_t::X) {
                rhs = clamp(context, rhs, -2.0f, 2.0f);
            } else if (i.clamp == clamp_t::B) {
                rhs = clamp(context, rhs, -1.0f, 1.0f);
            }
        }
        
        assert(!i.is_bx2);
        
        int regnum;
        const char* regname;
        if (i.is_reg_c) {
            regnum = i.control == control_mod_t::C0 ? 0 : 1;
            regname = "c";
        } else {
            regnum = i.reg_num;
            regname = i.reg == reg_type_t::H ? "h" : "r";
        }
        
        if (i.opcode.instr == fragment_op_t::IFE) {
            auto relation = makeCondition(context, i.condition);
            assert(relation);
            auto cond = context.make<Invocation>(FunctionName::any, relation);
            res.emplace_back(context.make<IfStubFragmentStatement>(cond, i.elseLabel, i.endifLabel));
        } else {
            if (i.opcode.instr == fragment_op_t::KIL) {
                Statement* st = context.make<DiscardStatement>();
                auto relation = makeCondition(context, i.condition);
                if (relation) {
                    auto cond = context.make<Invocation>(FunctionName::any, relation);
                    st = context.make<IfStatement>(cond, StV{dynamic_cast<Statement*>(st)}, StV{}, 0u);
                }
                res.emplace_back(st);
            } else {
                Expression* dest = context.make<Variable>(regname, context.make<IntegerLiteral>(regnum));
                rhs = appendCondition(context, i.condition, rhs, dest);
                dest = context.make<ComponentMask>(dest, i.dest_mask);
                if (!isScalarRhs) {
                    rhs = context.make<ComponentMask>(rhs, i.dest_mask);
                }
                auto assign = context.make<Assignment>(dest, rhs);
                res.emplace_back(assign);
                
                auto cassign = makeCAssignment(context, i.control, i.is_reg_c, dest);
                if (cassign) {
                    res.emplace_back(cassign);
                }
            }
        }
        
        return res;
    }
    
    std::tuple<int, int> GetLastRegisterNum(Expression* expr) {
        InfoCollectorVisitor visitor;
        expr->accept(&visitor);
        return { visitor.lastRegisterUsed(), visitor.lastHRegisterUsed() };
    }

    class VertexRefVisitor : public boost::static_visitor<Expression*> {
        ASTContext* _context;
        
    public:
        VertexRefVisitor(ASTContext* context) : _context(context) {}
        
        Expression* operator()(vertex_arg_address_ref x) const {
            Expression* var = _context->make<Variable>("a", _context->make<IntegerLiteral>(x.reg));
            var = _context->make<ComponentMask>(var, dest_mask_t{
                x.component == 0, 
                x.component == 1, 
                x.component == 2, 
                x.component == 3
            });
            return _context->make<BinaryOperator>(FunctionName::add, 
                                                  var, 
                                                  _context->make<IntegerLiteral>(x.displ));
        }
        
        Expression* operator()(int x) const {
            return _context->make<IntegerLiteral>(x);
        }
    };
    
    class VertexArgVisitor : public boost::static_visitor<Expression*> {
        ASTContext* _context;
        
    public:
        VertexArgVisitor(ASTContext* context) : _context(context) {}
        Expression* convert(std::string name, vertex_arg_ref_t ref, bool neg, bool abs, swizzle_t sw) const {
            auto refNum = apply_visitor(VertexRefVisitor(_context), ref);
            Expression* expr = _context->make<Variable>(name, refNum);
            if (abs)
                expr = _context->make<Invocation>(FunctionName::abs, expr);
            if (neg)
                expr = _context->make<UnaryOperator>(FunctionName::neg, expr);
            if (!sw.is_xyzw())
                expr = _context->make<Swizzle>(expr, sw);
            return expr;
        }
        
        Expression* operator()(vertex_arg_output_ref_t x) const {
            return convert("v_out", x.ref, false, false, swizzle_xyzw);
        }
        
        Expression* operator()(vertex_arg_input_ref_t x) const {
            return convert("v_in", x.ref, x.is_neg, x.is_abs, x.swizzle);
        }
        
        Expression* operator()(vertex_arg_const_ref_t x) const {
            return convert("constants.c", x.ref, x.is_neg, x.is_abs, x.swizzle);
        }
        
        Expression* operator()(vertex_arg_temp_reg_ref_t x) const {
            return convert("r", x.ref, x.is_neg, x.is_abs, x.swizzle);
        }
        
        Expression* operator()(vertex_arg_address_reg_ref_t x) const {
            return convert("a", x.a, false, false, swizzle_xyzw);
        }
        
        Expression* operator()(vertex_arg_cond_reg_ref_t x) const {
            return convert("c", x.c, false, false, swizzle_xyzw);
        }
        
        Expression* operator()(vertex_arg_label_ref_t x) const {
            return _context->make<IntegerLiteral>(x.l);
        }
        
        Expression* operator()(vertex_arg_tex_ref_t x) const {
            return _context->make<IntegerLiteral>(x.tex);
        }
    };
    
    std::vector<Statement*> MakeStatement(ASTContext& context, const VertexInstr& i, unsigned address) {
        VertexArgVisitor argVisitor(&context);
        auto arg = [&](int j) {
            return apply_visitor(argVisitor, i.args[j]);
        };
        
        Expression* rhs = nullptr, *lhs = nullptr;
        bool isScalarRhs = false;
        
        switch (i.operation) {
            case vertex_op_t::ADD: {
                rhs = context.make<BinaryOperator>(FunctionName::add, arg(1), arg(2));
                break;
            }
            case vertex_op_t::MAD: {
                auto mul = context.make<BinaryOperator>(FunctionName::mul, arg(1), arg(2));
                auto add = context.make<BinaryOperator>(FunctionName::add, mul, arg(3));
                rhs = add;
                break;
            }
            case vertex_op_t::ARL: {
                rhs = context.make<Invocation>(FunctionName::ivec4,
                                     context.make<Invocation>(FunctionName::floor, arg(1)));
                break;
            }
            case vertex_op_t::ARR: {
                rhs = context.make<Invocation>(FunctionName::ivec4,
                                     context.make<Invocation>(FunctionName::ceil, arg(1)));
                break;
            }
            case vertex_op_t::BRB:
            case vertex_op_t::BRI: {
                lhs = context.make<Variable>("nip", nullptr);
                rhs = arg(0);
                break;
            }
            case vertex_op_t::CLI:
            case vertex_op_t::CLB: {
                lhs = context.make<Variable>("void_var", nullptr);
                rhs = context.make<Invocation>(FunctionName::call, arg(0));
                break;
            }
            case vertex_op_t::COS: {
                rhs = context.make<Invocation>(FunctionName::cos, adaptToSingleScalar(context, arg(1)));
                break;
            }
            case vertex_op_t::DP3: {
                isScalarRhs = true;
                auto arg0 = context.make<ComponentMask>(arg(1), dest_mask_t{1, 1, 1, 0});
                auto arg1 = context.make<ComponentMask>(arg(2), dest_mask_t{1, 1, 1, 0});
                rhs = context.make<Invocation>(FunctionName::dot, arg0, arg1);
                break;
            }
            case vertex_op_t::DP4: {
                isScalarRhs = true;
                rhs = context.make<Invocation>(FunctionName::dot, arg(1), arg(2));
                break;
            }
            case vertex_op_t::DPH: {
                isScalarRhs = true;
                auto sw = context.make<ComponentMask>(arg(1), dest_mask_t{1, 1, 1, 0});
                auto xyz1 = context.make<Invocation>(FunctionName::vec4, sw, context.make<FloatLiteral>(1));
                rhs = context.make<Invocation>(FunctionName::dot, xyz1, arg(2));
                break;
            }
            case vertex_op_t::DST: {
                auto c0 = context.make<FloatLiteral>(1);
                auto c1 = context.make<BinaryOperator>(
                    FunctionName::mul,
                    context.make<ComponentMask>(arg(1), dest_mask_t{ 0, 1, 0, 0 }),
                    context.make<ComponentMask>(arg(2), dest_mask_t{ 0, 1, 0, 0 })
                );
                auto c2 = context.make<ComponentMask>(arg(1), dest_mask_t{ 0, 0, 1, 0 });
                auto c3 = context.make<ComponentMask>(arg(2), dest_mask_t{ 0, 0, 0, 1 });
                rhs = context.make<Invocation>(FunctionName::vec4, c0, c1, c2, c3);
                break;
            }
            case vertex_op_t::EXP:
            case vertex_op_t::EX2: {
                rhs = context.make<Invocation>(FunctionName::exp2, adaptToSingleScalar(context, arg(1)));
                break;
            }
            case vertex_op_t::FLR: {
                rhs = context.make<Invocation>(FunctionName::floor, arg(1));
                break;
            }
            case vertex_op_t::FRC: {
                rhs = context.make<Invocation>(FunctionName::fract, arg(1));
                break;
            }
            case vertex_op_t::LG2:
            case vertex_op_t::LOG: {
                rhs = context.make<Invocation>(FunctionName::lg2, adaptToSingleScalar(context, arg(1)));
                break;
            }
            case vertex_op_t::LIT: {
                auto c0 = context.make<FloatLiteral>(1);
                auto c1 = context.make<ComponentMask>(arg(1), dest_mask_t{ 1, 0, 0, 0 });
                auto mul = context.make<BinaryOperator>(
                    FunctionName::mul,
                    context.make<ComponentMask>(arg(1), dest_mask_t{ 0, 0, 0, 1 }),
                    context.make<Invocation>(
                        FunctionName::lg2,
                        context.make<ComponentMask>(arg(1), dest_mask_t{ 0, 1, 0, 0 })
                    )
                );
                auto c2 = context.make<TernaryOperator>(
                    context.make<BinaryOperator>(
                        FunctionName::gt,
                        context.make<ComponentMask>(arg(1), dest_mask_t{ 1, 0, 0, 0 }),
                        context.make<FloatLiteral>(0)
                    ),
                    context.make<Invocation>(FunctionName::exp2, mul),
                    context.make<FloatLiteral>(0)
                );
                auto c3 = context.make<FloatLiteral>(1);
                rhs = context.make<Invocation>(FunctionName::vec4, c0, c1, c2, c3);
                break;
            }
            case vertex_op_t::MIN: {
                rhs = context.make<Invocation>(FunctionName::min, arg(1), arg(2));
                break;
            }
            case vertex_op_t::MAX: {
                rhs = context.make<Invocation>(FunctionName::max, arg(1), arg(2));
                break;
            }
            case vertex_op_t::MOV: {
                break;
            }
            case vertex_op_t::MUL: {
                rhs = context.make<BinaryOperator>(FunctionName::mul, arg(1), arg(2));
                break;
            }
            case vertex_op_t::MVA: {
                auto x = context.make<Swizzle>(arg(1), swizzle_t{ swizzle2bit_t::X, swizzle2bit_t::Y });
                auto y = context.make<Swizzle>(arg(1), swizzle_t{ swizzle2bit_t::Z, swizzle2bit_t::W });
                auto sum = context.make<BinaryOperator>(FunctionName::add, x, y);
                auto sw = context.make<Swizzle>(sum, swizzle_t{
                    swizzle2bit_t::X, swizzle2bit_t::Y, swizzle2bit_t::X, swizzle2bit_t::Y 
                });
                rhs = context.make<Invocation>(FunctionName::clamp4i, sw);
                break;
            }
            case vertex_op_t::RCP: {
                auto exp = context.make<Invocation>(FunctionName::vec4, context.make<FloatLiteral>(-1));
                rhs = context.make<Invocation>(FunctionName::pow, adaptToSingleScalar(context, arg(1)), exp);
                break;
            }
            case vertex_op_t::RSQ: {
                rhs = context.make<Invocation>(FunctionName::inversesqrt, adaptToSingleScalar(context, arg(1)));
                break;
            }
            case vertex_op_t::SEQ: {
                auto func = context.make<Invocation>(FunctionName::equal, arg(1), arg(2));
                rhs = context.make<Invocation>(FunctionName::vec4, func);
                break;
            }
            case vertex_op_t::SFL: {
                rhs = context.make<Invocation>(FunctionName::vec4,
                    context.make<FloatLiteral>(0), context.make<FloatLiteral>(0),
                    context.make<FloatLiteral>(0), context.make<FloatLiteral>(0)
                );
                break;
            }
            case vertex_op_t::SGE: {
                auto func = context.make<Invocation>(FunctionName::greaterThanEqual, arg(1), arg(2));
                rhs = context.make<Invocation>(FunctionName::vec4, func);
                break;
            }
            case vertex_op_t::SGT: {
                auto func = context.make<Invocation>(FunctionName::greaterThan, arg(1), arg(2));
                rhs = context.make<Invocation>(FunctionName::vec4, func);
                break;
            }
            case vertex_op_t::SIN: {
                rhs = context.make<Invocation>(FunctionName::sin, adaptToSingleScalar(context, arg(1)));
                break;
            }
            case vertex_op_t::SLE: {
                auto func = context.make<Invocation>(FunctionName::lessThanEqual, arg(1), arg(2));
                rhs = context.make<Invocation>(FunctionName::vec4, func);
                break;
            }
            case vertex_op_t::SLT: {
                auto func = context.make<Invocation>(FunctionName::lessThan, arg(1), arg(2));
                rhs = context.make<Invocation>(FunctionName::vec4, func);
                break;
            }
            case vertex_op_t::SNE: {
                auto func = context.make<Invocation>(FunctionName::notEqual, arg(1), arg(2));
                rhs = context.make<Invocation>(FunctionName::vec4, func);
                break;
            }
            case vertex_op_t::SSG: {
                rhs = context.make<Invocation>(FunctionName::sign, arg(1));
                break;
            }
            case vertex_op_t::STR: {
                rhs = context.make<Invocation>(FunctionName::vec4, 
                    context.make<FloatLiteral>(1), context.make<FloatLiteral>(1),
                    context.make<FloatLiteral>(1), context.make<FloatLiteral>(1)
                );
                break;
            }
            case vertex_op_t::TXL: {
                auto txl = dynamic_cast<IntegerLiteral*>(arg(2));
                assert(txl);
                auto func = txl->value() == 0 ? FunctionName::txl0
                          : txl->value() == 1 ? FunctionName::txl1
                          : txl->value() == 2 ? FunctionName::txl2
                          : FunctionName::txl3;
                rhs = context.make<Invocation>(func, arg(1));
                break;
            }
            case vertex_op_t::NOP: {
                lhs = context.make<WhileStatement>(context.make<Variable>("false", nullptr), StV{});
                break;
            }
            default: assert(false);
        }
        
        bool isRegC = i.op_count > 1 && boost::get<vertex_arg_cond_reg_ref_t>(&i.args[0]);
        
        auto makeMaskedAssignment = [&] (auto lhs, auto rhs) {
            if (isScalarRhs) {
                if (i.mask.arity() != 1) {
                    rhs = context.make<ComponentMask>(context.make<Invocation>(FunctionName::vec4, rhs), i.mask);
                }
            } else {
                if (i.mask.arity() != 4) {
                    rhs = context.make<ComponentMask>(rhs, i.mask);
                }
            }
            if (i.mask.arity() != 4) {
                lhs = context.make<ComponentMask>(lhs, i.mask);
            }
            return context.make<Assignment>(lhs, rhs);
        };
        
        std::vector<Statement*> vec;
        
        if (i.operation == vertex_op_t::MOV) {
            if (i.op_count == 2) {
                auto rhs = appendCondition(context, i.condition, arg(1), arg(0));
                vec.emplace_back(makeMaskedAssignment(arg(0), rhs));
            } else {
                assert(i.op_count == 3);
                auto rhs = appendCondition(context, i.condition, arg(2), arg(0));
                vec.emplace_back(makeMaskedAssignment(arg(0), rhs));
                vec.emplace_back(makeMaskedAssignment(arg(1), rhs));
            }
        } else if (i.operation == vertex_op_t::NOP) {
            vec.emplace_back(dynamic_cast<Statement*>(lhs));
        } else if (i.operation == vertex_op_t::BRI || i.operation == vertex_op_t::BRB) {
            auto assign = context.make<Assignment>(lhs, rhs);
            auto relation = makeCondition(context, i.condition);
            if (relation) {
                auto cond = context.make<Invocation>(FunctionName::any, relation);
                auto ifSt = context.make<IfStatement>(cond, StV{assign, context.make<BreakStatement>()}, StV{}, 0);
                vec.emplace_back(ifSt);
            } else {
                vec.emplace_back(assign);
                vec.emplace_back(context.make<BreakStatement>());
            }
        } else {
            assert(rhs);
            if (!lhs) {
                lhs = arg(0);
            }
            rhs = appendCondition(context, i.condition, rhs, lhs);
            vec.emplace_back(makeMaskedAssignment(lhs, rhs));
            if (i.operation == vertex_op_t::BRI) {
                vec.emplace_back(context.make<BreakStatement>());
            }
        }

        auto cassign = makeCAssignment(context, i.control, isRegC, lhs);
        if (cassign) {
            vec.emplace_back(cassign);
        }
        
        for (auto& assignment : vec) {
            assignment->address(address);
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
    
    std::vector<Statement*> RewriteBranches(
        ASTContext& context,
        std::vector<Statement*> uniqueStatements) {
        DoesContainBranchVisitor visitor;
        for (auto& st : uniqueStatements) {
            st->accept(&visitor);
        }
        if (!visitor.result)
            return uniqueStatements;
        auto sts = uniqueStatements;
        auto sw = context.make<SwitchStatement>(context.make<Variable>("nip", nullptr));
        
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
                curCase.push_back(context.make<Assignment>(context.make<Variable>("nip", nullptr),
                                                 context.make<IntegerLiteral>(-1)));
            }
            sw->addCase(curCase.front()->address(), curCase);
            curCase.clear();
        }
        
        std::vector<Statement*> res = {
            context.make<Assignment>(context.make<Variable>("nip", nullptr),
                                     context.make<IntegerLiteral>(0)),
            context.make<WhileStatement>(
                context.make<BinaryOperator>(FunctionName::ne,
                                             context.make<Variable>("nip", nullptr),
                                             context.make<IntegerLiteral>(-1)),
                StV{sw})};
        return res;
    }
    
    void UsedConstsVisitor::visit(Variable* variable) {
        if (variable->name() == "constants.c") {
            if (auto index = dynamic_cast<IntegerLiteral*>(variable->index())) {
                _consts.insert(index->value());
            }
        }
    }
    
    const std::set<unsigned>& UsedConstsVisitor::consts() {
        return _consts;
    }
    
    bool fixFirstIfStub(ASTContext& context, std::vector<Statement*>& statements) {
        auto it = boost::find_if(statements, [](auto s) {
            auto stub = dynamic_cast<IfStubFragmentStatement*>(s);
            if (!stub)
                return false;
            return stub->trueBlock().empty();
        });
        if (it == end(statements))
            return false;
        auto stub = (IfStubFragmentStatement*)*it;
        auto trueStart = it + 1;
        auto trueEnd = boost::find_if(statements, [&](auto s) {
            return s->address() / 16 == stub->elseLabel();
        });
        auto falseEnd = boost::find_if(statements, [&](auto s) {
            return s->address() / 16 == stub->endifLabel();
        });
        if (falseEnd == end(statements)) {
            assert(stub->endifLabel() > statements.back()->address() / 16);
        }
        assert(trueEnd != end(statements));
        std::vector<Statement*> trueBlock(trueStart, trueEnd);
        std::vector<Statement*> falseBlock(trueEnd, falseEnd);
        
        statements.erase(trueStart, falseEnd);
        while (fixFirstIfStub(context, trueBlock)) {}
        while (fixFirstIfStub(context, falseBlock)) {}
        stub->setTrueBlock(trueBlock);
        stub->setFalseBlock(falseBlock);
        return true;
    }
    
    std::vector<Statement*> RewriteIfStubs(ASTContext& context,
                                           std::vector<Statement*> statements) {
        while (fixFirstIfStub(context, statements)) { }
        return statements;
    }
}
