/*
 *  Implementation of "forStmt"
 *
 *  Please refer to ast/ast.hpp for the definition.
 */

#include "ast/ast.hpp"
#include "ast/visitor.hpp"
#include "config.hpp"

using namespace mind;
using namespace mind::ast;

/* Creates a new ForStmt node.
 *
 * PARAMETERS:
 *   cond    - the test expression
 *   body    - the loop body
 *   l       - position in the source text
 */
ForStmt::ForStmt(Expr *init, Expr *cond, Expr *update, Statement *body,
                 Location *l) {

    setBasicInfo(FOR_STMT, l);
    this->init = (ASTNode *)init;
    this->update = update;
    first_condition = condition = cond;
    loop_body = body;
}
ForStmt::ForStmt(Statement *init, Expr *cond, Expr *update, Statement *body,
                 Location *l) {

    setBasicInfo(FOR_STMT, l);
    this->init = (ASTNode *)init;
    this->update = update;
    first_condition = condition = cond;
    loop_body = body;
}

ForStmt::ForStmt(Expr *cond, Statement *body, Location *l){
    setBasicInfo(FOR_STMT, l);
    init = NULL;
    update = first_condition = NULL;
    condition = cond;
    loop_body = body;
}
/* Visits the current node.
 *
 * PARAMETERS:
 *   v       - the visitor
 */
void ForStmt::accept(Visitor *v) { v->visit(this); }

/* Prints the current AST node.
 *
 * PARAMETERS:
 *   os      - the output stream
 */
void ForStmt::dumpTo(std::ostream &os) {
    ASTNode::dumpTo(os);
    if (init != NULL) {
        newLine(os);
        os << "init " << init << ")";
    }

    if (first_condition != NULL) {
        newLine(os);
        os << "first " << first_condition << ")";
    }

    if (condition != NULL){
        newLine(os);
        os << "condition " << condition << ")";
    }
    if (update != NULL) {
        newLine(os);
        os << "update " << update << ")";
    }
    newLine(os);
    os << loop_body << ")";
    decIndent(os);
}