#ifndef _UBSP_SYNTAX_
#define _UBSP_SYNTAX_

#include "ubsp-fwd.h"
#include "operators.h"
#include <map>
#include <set>
#include <string>
#include <list>

NAMESPACE_UBSP_BEGIN;

// abstract syntax node
struct syntax_node_i
{
    virtual ~syntax_node_i() {}
    virtual void process(syntax_processor_i&) const = 0;
};

#define PROCESS_IMPL virtual void process(syntax_processor_i& p) const override { p.process(*this); }

// abstract declaration node:
struct declaration_i : syntax_node_i {
    mutable decl_p next = nullptr; // linked list of definitions
};

// abstract statement
struct statement_i : syntax_node_i {
    mutable stmt_p next = nullptr; // linked list of statements
};

// abstract expression node
struct expression_i : syntax_node_i {
    mutable expr_p next = nullptr; // linked list of expressions
    virtual expr_priority_t get_priority() const = 0;
};

#define EXPR_IMPL(prio) \
    PROCESS_IMPL; \
    virtual expr_priority_t get_priority() const override { return prio; }

struct argument_t
{
    mutable args_p next = nullptr; // linked list of arguments
    name_t name;
};

// generic lvalue (i.e. variable)
struct lvalue_t
{
    name_t name;
    expr_p index;
};

// generic procedure/function call:
struct func_call_t
{
    name_t name;
    expr_p args;
};

//
// expressions:
//

struct const_expr_t : expression_i
{
    number_t value;
    EXPR_IMPL(IDENT_PRIORITY);
};

struct lval_expr_t : expression_i
{
    lvalue_t lval;
    EXPR_IMPL(IDENT_PRIORITY);
};

struct chng_expr_t : expression_i
{
    lvalue_t lval;
    expr_p value;
    binary_oper_type_t type;
    EXPR_IMPL(CHNG_PRIORITY);
};

struct incr_expr_t : expression_i
{
    lvalue_t lval;
    binary_oper_type_t type;
    bool postfix = false;
    EXPR_IMPL(postfix ? POSTFIX_PRIORITY : PREFIX_PRIORITY);
};

struct unary_oper_t : expression_i
{
    expr_p operand;
    unary_oper_type_t type;
    EXPR_IMPL(PREFIX_PRIORITY);
};

struct binary_oper_t : expression_i 
{
    expr_p left, right;
    binary_oper_type_t type;
    EXPR_IMPL(get_operator_info(type).priority);
};

struct cond_expr_t : expression_i 
{
    expr_p cond;
    expr_p expr_true;
    expr_p expr_false;
    EXPR_IMPL(COND_PRIORITY);
};

struct call_expr_t : expression_i
{
    func_call_t call;
    EXPR_IMPL(IDENT_PRIORITY);
};

//
// statements:
//

struct break_stmt_t : statement_i
{
    PROCESS_IMPL;
};

struct continue_stmt_t : statement_i
{
    PROCESS_IMPL;
};

struct return_stmt_t : statement_i
{
    expr_p value;
    PROCESS_IMPL;
};

struct expr_stmt_t : statement_i
{
    expr_p expr;
    PROCESS_IMPL;
};

struct cond_stmt_t : statement_i
{
    expr_p cond;
    stmt_p stmt_true;
    stmt_p stmt_false;
    PROCESS_IMPL;
};

struct loop_stmt_t : statement_i
{
    expr_p cond;
    stmt_p body;
    bool pre_check;
    PROCESS_IMPL;
};

struct for_loop_stmt_t : statement_i
{
    expr_p cond;
    stmt_p body;
    stmt_p incr;
    PROCESS_IMPL;
};

struct load_stmt_t : statement_i
{
    lvalue_t lval;
    func_call_t call;
    PROCESS_IMPL;
};

//
// declarations
//

struct root_node_t : declaration_i
{
    PROCESS_IMPL;
};

struct stmt_decl_t : declaration_i
{
    stmt_p stmt;
    PROCESS_IMPL;
};

struct func_defn_t : declaration_i
{
    name_t name;
    args_p args;
    stmt_p body;
    PROCESS_IMPL;
};

// memory management:
// fixed size buffer is allocated for all nodes

constexpr size_t MAX_NODE_SIZE = sizeof(chng_expr_t);

union node_buffer_t
{
    char space[MAX_NODE_SIZE];
    node_buffer_t *next_free;
};

//
// abstract syntax tree:
//

class syntax_t
{
private:
    friend class syntax_loader_t;
    std::set<std::string> idents;
    root_node_t root;
    decl_p last;

    std::list<node_buffer_t> pool;
    node_buffer_t *first_free;
    
public:

    syntax_t();

    void load(const char *syntax_file);
    void process(syntax_processor_i& processor) {
        root.process(processor);
    }

    decl_p get_tree_root() const { return &root; }



};


NAMESPACE_UBSP_END;

#endif // _UBSP_SYNTAX_