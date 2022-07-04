/*
 *  Definition of the two AST translation passes.
 *  第二步定义
 * 
 */

#ifndef __MIND_TRANSLATION__INTERNAL__
#define __MIND_TRANSLATION__INTERNAL__

#include "3rdparty/vector.hpp"
#include "ast/visitor.hpp"
#include "define.hpp"

// we assume the target machine is a 32-bit machine for simplicity.
#define POINTER_SIZE 4
#define WORD_SIZE 4

namespace mind {

class Translation : public ast::Visitor {
  public:
    Translation(tac::TransHelper *);

    virtual void visit(ast::Program *);
    virtual void visit(ast::FuncDefn *);
    virtual void visit(ast::AssignExpr *);
    virtual void visit(ast::CompStmt *);//语句块
    virtual void visit(ast::ExprStmt *);
    virtual void visit(ast::IfStmt *);//if分支
    virtual void visit(ast::IfExpr *);//if条件
    virtual void visit(ast::ReturnStmt *);
    virtual void visit(ast::CallExpr *);//函数调用
    virtual void visit(ast::EquExpr *);//等于
    virtual void visit(ast::NeqExpr *);//不等
    virtual void visit(ast::GrtExpr *);//大于
    virtual void visit(ast::GeqExpr *);//大于等于
    virtual void visit(ast::AndExpr *);//与
    virtual void visit(ast::OrExpr *);//或
    virtual void visit(ast::AddExpr *);
    virtual void visit(ast::SubExpr *);//减
    virtual void visit(ast::MulExpr *);//乘
    virtual void visit(ast::DivExpr *);//除
    virtual void visit(ast::ModExpr *);//模
    virtual void visit(ast::IndexExpr *);//数组
    virtual void visit(ast::IntConst *);
    virtual void visit(ast::NegExpr *);
    virtual void visit(ast::BitNotExpr *);//按位取反
    virtual void visit(ast::NotExpr *);//逻辑非
    virtual void visit(ast::LvalueExpr *);
    virtual void visit(ast::VarRef *);
    virtual void visit(ast::VarDecl *);
    virtual void visit(ast::WhileStmt *);
    virtual void visit(ast::ForStmt *);//for循环
    virtual void visit(ast::BreakStmt *);
    virtual void visit(ast::ContinueStmt *);//continue条件

    virtual ~Translation() {}

  private:
    tac::TransHelper *tr;
    tac::Label current_break_label, current_continue_label;
    // TODO: label for continue
};
} // namespace mind

#endif // __MIND_TRANSLATION__INTERNAL__
