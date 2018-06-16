#ifndef _UBSP_FWD_
#define _UBSP_FWD_

#include <stdint.h>

#define NAMESPACE_UBSP_BEGIN namespace ubsp {
#define NAMESPACE_UBSP_END   }

NAMESPACE_UBSP_BEGIN;

typedef int64_t number_t;
typedef const char *name_t;

constexpr size_t MAX_IDENT_LEN = 256;

class machine_t; // virtual machine
class syntax_t; // abstract syntax tree
class syntax_loader_t; // syntax parsing runtime

struct syntax_node_i;
struct expression_i;
struct statement_i;

struct const_expr_t;
struct lval_expr_t;
struct chng_expr_t;
struct incr_expr_t;
struct unary_oper_t;
struct binary_oper_t;
struct cond_expr_t;
struct call_expr_t;
struct root_stmt_t;
struct exit_stmt_t;
struct expr_stmt_t;
struct cond_stmt_t;
struct loop_stmt_t;
struct for_loop_stmt_t;
struct load_stmt_t;
struct func_defn_t;

struct lvalue_t;
struct func_call_t;

typedef const statement_i *stmt_p;
typedef const expression_i *expr_p;

struct syntax_processor_i
{
    virtual void process(const const_expr_t *) = 0;
    virtual void process(const lval_expr_t *) = 0;
    virtual void process(const chng_expr_t *) = 0;
    virtual void process(const incr_expr_t *) = 0;
    virtual void process(const unary_oper_t *) = 0;
    virtual void process(const binary_oper_t *) = 0;
    virtual void process(const cond_expr_t *) = 0;
    virtual void process(const call_expr_t *) = 0;
    virtual void process(const root_stmt_t *) = 0;
    virtual void process(const exit_stmt_t *) = 0;
    virtual void process(const expr_stmt_t *) = 0;
    virtual void process(const cond_stmt_t *) = 0;
    virtual void process(const loop_stmt_t *) = 0;
    virtual void process(const for_loop_stmt_t *) = 0;
    virtual void process(const load_stmt_t *) = 0;
    virtual void process(const func_defn_t *) = 0;
};


// binary operations
enum class binary_oper_type_t {
    NOP, // ,

    ADD, // +
    SUB, // -
    MUL, // *
    DIV, // /
    MOD, // %

    SHL, // <<
    SHR, // >>

    BAND,// &
    BOR, // |
    XOR, // ^

    AND, // &&
    OR,  // ||

    EQ, // ==
    NE, // !=
    LT, // <
    LE, // <=
    GT, // >
    GE, // >=

    LAST
};

// unary operations
enum class unary_oper_type_t {
    NEG, // -
    NOT, // !
    BNOT,// ~

    LAST
};

// scope exit
enum class scope_exit_type_t {
    RETURN,
    BREAK,
    CONTINUE,

    LAST
};

constexpr size_t NUM_BINARY_OPER_TYPES = size_t(binary_oper_type_t::LAST);
constexpr size_t NUM_UNARY_OPER_TYPES = size_t(unary_oper_type_t::LAST);
constexpr size_t NUM_SCOPE_EXIT_TYPES = size_t(scope_exit_type_t::LAST);

enum expr_priority_t {
    NOP_PRIORITY = 0,
    CHNG_PRIORITY,
    COND_PRIORITY,
    OR_PRIORITY,
    AND_PRIORITY,
    BOR_PRIORITY,
    XOR_PRIORITY,
    BAND_PRIORITY,
    EQ_PRIORITY,
    CMP_PRIORITY,
    SHL_PRIORITY,
    ADD_PRIORITY,
    MUL_PRIORITY,
    PREFIX_PRIORITY,
    POSTFIX_PRIORITY,
    IDENT_PRIORITY = 100,
};

NAMESPACE_UBSP_END;

#endif // _UBSP_FWD_