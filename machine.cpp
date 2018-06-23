#include "machine.h"
#include "syntax.h"
#include "operators.h"
#include "utils.h"
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
    // evaluate arguments:
    number_t args[30];
    int args_num = 0;

    for (expr_p arg = call.args; arg; arg = arg->next)
        args[args_num++] = eval(arg);

    // find function:
    auto user_defined_func = funcs.find(call.name);
    if (user_defined_func != funcs.end()) {
        return this->call(*user_defined_func->second, args_num, args);
    }
    
    // TODO support native functions
    throw func_not_defined{ call.name };
}

number_t machine_t::call(const func_defn_t& func, int args_num, number_t args[])
{
    // check args:
    int required_args_num = 0;
    for (args_p arg = func.args; arg; arg = arg->next)
        required_args_num++;
    if (args_num != required_args_num) {
        std::cerr << "Wrong number of arguments at func '" << func.name << "': " 
            << required_args_num << " required, " << args_num << " provided" << std::endl;
        throw "Wrong number of arguments";
    }

    // fill local scope:
    var_scope_t local_scope;
    for (args_p arg = func.args; arg; arg = arg->next)
        local_scope[arg->name] = *args++;

    try {
        var_scope_t *prev_scope = scope;
        on_exit_scope([this, prev_scope] {
            scope = prev_scope;
        });
        
        scope = &local_scope;
        exec(func.body);
        value = 0; // default return value
    }
    catch (break_exception) {
        std::cerr << "Uncaught break at func: " << func.name << std::endl;
    }
    catch (continue_exception) {
        std::cerr << "Uncaught continue at func: " << func.name << std::endl;
    }
    catch (return_exception r) {
        value = r.value;
    }

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
    throw return_exception{ eval(node.value) };
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