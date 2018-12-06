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
};

// abstract declaration node:
struct declaration_i : syntax_node_i {
    mutable decl_p next = nullptr; // linked list of definitions
    virtual void process(decl_processor_i&) const = 0;
};

// abstract statement
struct statement_i : syntax_node_i {
    mutable stmt_p next = nullptr; // linked list of statements
    virtual void process(stmt_processor_i&) const = 0;
};

// abstract expression node
struct expression_i : syntax_node_i {
    mutable expr_p next = nullptr; // linked list of expressions
    virtual expr_priority_t get_priority() const = 0;
    virtual void process(expr_processor_i&) const = 0;
};

#define PROCESS_DECL virtual void process(decl_processor_i& p) const override { p.process(*this); }
#define PROCESS_STMT virtual void process(stmt_processor_i& p) const override { p.process(*this); }

#define EXPR_IMPL(prio) \
    virtual void process(expr_processor_i& p) const override { p.process(*this); } \
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
    PROCESS_STMT;
};

struct continue_stmt_t : statement_i
{
    PROCESS_STMT;
};

struct return_stmt_t : statement_i
{
    expr_p value;
    PROCESS_STMT;
};

struct expr_stmt_t : statement_i
{
    expr_p expr;
    PROCESS_STMT;
};

struct cond_stmt_t : statement_i
{
    expr_p cond;
    stmt_p stmt_true;
    stmt_p stmt_false;
    PROCESS_STMT;
};

struct loop_stmt_t : statement_i
{
    expr_p cond;
    stmt_p body;
    bool pre_check;
    PROCESS_STMT;
};

struct for_loop_stmt_t : statement_i
{
    stmt_p init;
    expr_p cond;
    stmt_p incr;
    stmt_p body;
    PROCESS_STMT;
};

struct load_stmt_t : statement_i
{
    lvalue_t lval;
    func_call_t call;
    PROCESS_STMT;
};

//
// declarations
//

struct root_node_t : declaration_i
{
    PROCESS_DECL;
};

struct stmt_decl_t : declaration_i
{
    stmt_p stmt;
    PROCESS_DECL;
};

struct infer_defn_t : declaration_i
{
    name_t scope;
    name_t name;
    stmt_p body;
    PROCESS_DECL;
};

struct func_defn_t : declaration_i
{
    name_t name;
    args_p args;
    stmt_p body;
    PROCESS_DECL;
};

struct import_decl_t : declaration_i
{
    name_t name;
    root_node_t root;
    PROCESS_DECL;
};

// memory management:
// fixed size buffer is allocated for all nodes

constexpr size_t MAX_NODE_SIZE = sizeof(for_loop_stmt_t);

union node_buffer_t
{
    char space[MAX_NODE_SIZE];
    node_buffer_t *next_free;
};

//
// extended information:
//

struct function_info_t
{
    function_info_t() = default;
    function_info_t(const func_defn_t& func) : 
        name(func.name), body(func.body)
    {
        for (args_p arg = func.args; arg; arg = arg->next)
            args.push_back(arg->name);
    }

    name_t name = nullptr;
    stmt_p body = nullptr;
    std::list<name_t> args;
    std::set<name_t> reads;
    std::set<name_t> loads;
    std::set<name_t> writes;
    std::set<name_t> globals;
};

struct variable_info_t
{
    variable_info_t(name_t n = nullptr) : name(n) {}

    const name_t name;
    stmt_p infer = nullptr;
    name_t func = nullptr;
    //int ndims = 0;
};

typedef std::map<name_t, stmt_p> infer_map_t;

//
// abstract syntax tree:
//

class syntax_t
{
public:

    syntax_t();

    void load(const char *syntax_file);
    void load(const import_decl_t& import);
    void analyze();
    void process(decl_processor_i& processor) {
        root.process(processor);
    }

    decl_p get_tree_root() const { return &root; }

    void find_modules(const char *start_path);

    const function_info_t *get_function(name_t name) const;
    const variable_info_t *get_variable(name_t name) const;

private:
    friend class syntax_loader_t;
    friend class syntax_analyzer_t;
    friend class machine_t;

    std::set<std::string> idents;
    std::set<name_t> modules;
    root_node_t root;

    std::list<node_buffer_t> pool;
    node_buffer_t *first_free;
    char modules_path[260];

    std::list<stmt_p> global_body;
    function_info_t global_scope;
    std::map<name_t, function_info_t> function_info;
    std::map<name_t, variable_info_t> variable_info;
    std::map<name_t, infer_map_t> scoped_infers;

    name_t get_ident(const char *name) {
        auto i = idents.emplace(name);
        return i.first->c_str();
    }
    
};

struct undef_module_error {
    name_t name;
};

NAMESPACE_UBSP_END;

#endif // _UBSP_SYNTAX_