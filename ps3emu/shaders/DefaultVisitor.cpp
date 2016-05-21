#include "DefaultVisitor.h"

namespace ShaderRewriter {
    void DefaultVisitor::visit(ShaderRewriter::FloatLiteral* literal) { }
    
    void DefaultVisitor::visit(BinaryOperator* op) {
        op->args().at(0)->accept(this);
        op->args().at(1)->accept(this);
    }
    
    void DefaultVisitor::visit(UnaryOperator* op) {
        op->args().at(0)->accept(this);
    }
    
    void DefaultVisitor::visit(Swizzle* swizzle) {
        swizzle->expr()->accept(this);
    }
    
    void DefaultVisitor::visit(Variable* ref) { }
    
    void DefaultVisitor::visit(IntegerLiteral* ref) { }
    
    void DefaultVisitor::visit(ComponentMask* mask) {
        mask->expr()->accept(this);
    }
    
    void DefaultVisitor::visit(IfStatement* ifst) {
        ifst->condition()->accept(this);
        for (auto& st : ifst->statements()) {
            st->accept(this);
        }
    }
    
    void DefaultVisitor::visit(Assignment* assignment) {
        assignment->dest()->accept(this);
        assignment->expr()->accept(this);
    }
    
    void DefaultVisitor::visit(Invocation* invocation) {
        for (auto& arg : invocation->args()) {
            arg->accept(this);
        }
    }
    
    void DefaultVisitor::visit(SwitchStatement* sw) {
        sw->switchOn()->accept(this);
        for (auto& c : sw->cases()) {
            for (auto& st : c.body) {
                st->accept(this);
            }
        }
    }
    
    void DefaultVisitor::visit(BreakStatement* sw) { }
    void DefaultVisitor::visit(DiscardStatement* sw) { }
    
    void DefaultVisitor::visit(WhileStatement* sw) {
        sw->condition()->accept(this);
        for (auto st : sw->body()) {
            st->accept(this);
        }
    }
    
    void DefaultVisitor::visit(TernaryOperator* ternary) {
        visit((Invocation*)ternary);
    }
}