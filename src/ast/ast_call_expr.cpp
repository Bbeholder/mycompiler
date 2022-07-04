/*
 *  函数调用实现
 *
 *  Please refer to ast/ast.hpp for the definition.
 *
 */

#include "ast/ast.hpp"
#include "ast/visitor.hpp"
#include "config.hpp"

using namespace mind;
using namespace mind::ast;

CallExpr::CallExpr(ExprList *expr_list, std::string func, Location *l){
    setBasicInfo(CALL_EXPR, l);

    this->func = func;
    this->expr_list = expr_list;
}

void CallExpr::accept(Visitor *v) { v->visit(this); }

void CallExpr::dumpTo(std::ostream &os) {
    ASTNode::dumpTo(os);
    newLine(os);

    os << func;

    for(auto expr : *expr_list){
        newLine(os);
        os << expr;
    }
    decIndent(os);
}
