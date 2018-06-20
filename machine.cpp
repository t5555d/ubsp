#include "machine.h"
#include "syntax.h"
#include "operators.h"
#include <iostream>

NAMESPACE_UBSP_BEGIN;

struct break_exception {};
struct continue_exception {};
struct return_exception { number_t value; };

machine_t::machine_t()
{
    scope = &global_scope;
}

//
// interface functions
//

number_t machine_t::get(const lvalue_t& lval)
{
    // TODO support indexing
    return (*scope)[lval.name];
}

void machine_t::put(const lvalue_t& lval, number_t n)
{
    // TODO support indexing
    (*scope)[lval.name] = n;
}

number_t machine_t::eval(expr_p expr, number_t default_value)
{
    if (!expr) return default_value;
    expr->process(*this);
    return value;
}

void machine_t::exec(stmt_p stmt)
{
    for ( ; stmt; stmt = stmt->next)
        stmt->process(*this);
}

number_t machine_t::call(const func_call_t& call)
{
    var_scope_t *prev_scope = scope;
    try {
        auto func = funcs.at(call.name);
        var_scope_t local_scope;
        scope = &local_scope;

        exec(func->body);
        value = 0;
    }
    catch (const std::out_of_range& ex) {
        std::cerr << "No such func: " << call.name << std::endl;
        throw ex;
    }
    catch (break_exception) {
        std::cerr << "Uncaught break at func: " << call.name << std::endl;
    }
    catch (continue_exception) {
        std::cerr << "Uncaught continue at func: " << call.name << std::endl;
    }
    catch (return_exception) {
        // pass
    }

    scope = prev_scope;
    return value;
}

//
// expressions
//

void machine_t::process(const const_expr_t& node)
{
    value = node.value;
}

void machine_t::process(const lval_expr_t& node)
{
    value = get(node.lval);
}

void machine_t::process(const call_expr_t& node)
{
    value = call(node.call);
}

void machine_t::process(const chng_expr_t& node)
{
    node.value->process(*this);
    auto oper = get_operator_info(node.type);
    if (oper.calc)
        value = oper.calc(get(node.lval), value);
    put(node.lval, value);
}

void machine_t::process(const incr_expr_t& node)
{
    auto oper = get_operator_info(node.type);
    number_t v = get(node.lval);
    if (node.postfix) value = v;
    v = oper.calc(v, 1);
    put(node.lval, v);
    if (!node.postfix) value = v;
}

void machine_t::process(const unary_oper_t& node)
{
    auto oper = get_operator_info(node.type);
    value = oper.calc(eval(node.operand), 0);
}

void machine_t::process(const binary_oper_t& node)
{
    auto oper = get_operator_info(node.type);
    number_t left = eval(node.left);
    number_t right = eval(node.right);
    value = oper.calc(left, right);
}

void machine_t::process(const cond_expr_t& node)
{
    value = eval(eval(node.cond) ? node.expr_true : node.expr_false);
}

//
// statements
//

void machine_t::process(const break_stmt_t& node)
{
    throw break_exception{};
}

void machine_t::process(const continue_stmt_t& node)
{
    throw continue_exception{};
}

void machine_t::process(const return_stmt_t& node)
{
    value = eval(node.value);
    throw return_exception{};
}

void machine_t::process(const expr_stmt_t& node)
{
    eval(node.expr);
}

void machine_t::process(const cond_stmt_t& node)
{
    exec(eval(node.cond) ? node.stmt_true : node.stmt_false);
}

void machine_t::process(const loop_stmt_t& node)
{
    if (node.pre_check && !eval(node.cond))
        return;
    do {
        try {
            exec(node.body);
        }
        catch (continue_exception) {
            continue;
        }
        catch (break_exception) {
            break;
        }
    } while (eval(node.cond));
}

void machine_t::process(const for_loop_stmt_t& node)
{
    for (; eval(node.cond, 1); exec(node.incr)) {
        try {
            exec(node.body);
        }
        catch (continue_exception) {
            continue;
        }
        catch (break_exception) {
            break;
        }
    }
}

void machine_t::process(const load_stmt_t& node)
{
    put(node.lval, call(node.call));
}

//
// declarations
//

void machine_t::process(const root_node_t& node)
{
    for (decl_p decl = node.next; decl; decl = decl->next)
        decl->process(*this);
}

void machine_t::process(const stmt_decl_t& node)
{
    exec(node.stmt);
}

void machine_t::process(const func_defn_t& node)
{
    funcs[node.name] = &node;
}

NAMESPACE_UBSP_END;