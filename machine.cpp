#include "machine.h"
#include "syntax.h"
#include "operators.h"
#include "utils.h"
#include <iostream>

#include "syntax-output.h"

NAMESPACE_UBSP_BEGIN;

struct break_exception {};
struct continue_exception {};
struct return_exception { number_t value; };

syntax_output_t debug(std::cerr);

machine_t::machine_t()
{
    scope = &global_scope;
}

//
// interface functions
//

int machine_t::eval_args(number_t argv[MAX_ARGS], expr_p expr)
{
    int argc = 0;
    for (expr_p arg = expr; arg; arg = arg->next)
        argv[argc++] = eval(arg);
    return argc;
}

array_t& machine_t::find(name_t name)
{
    auto var = scope->find(name);
    if (var != scope->end())
        return var->second;

    var = global_scope.find(name);
    if (var != global_scope.end())
        return var->second;

    var = memory_scope.find(name);
    if (var != memory_scope.end()) {
        auto inserted = global_scope.emplace(name, std::move(var->second));
        memory_scope.erase(var);
        return inserted.first->second;
    }

    auto infer = infers.find(name);
    if (infer != infers.end()) {
        number_t value = eval(infer->second);
        auto inserted = scope->emplace(name, value);
        return inserted.first->second;
    }

    throw undef_var_error{ name };
}

number_t machine_t::get(const lvalue_t& lval)
{
    array_t& value = find(lval.name);

    number_t index[MAX_ARGS];
    int ndims = eval_args(index, lval.index);

    return value.get(ndims, index);
}

void machine_t::put(const lvalue_t& lval, number_t n)
{
    debug << "    " << lval << " = " << n << std::endl;

    number_t index[MAX_ARGS];
    int ndims = eval_args(index, lval.index);

    auto var = scope->find(lval.name);
    if (var == scope->end()) {
        scope->emplace(lval.name, array_t(ndims, index, n));
    }
    else {
        var->second.put(ndims, index, n);
    }
}

number_t machine_t::eval(expr_p expr, number_t default_value)
{
    if (!expr) return default_value;
    expr->process(*this);
    return value;
}

void machine_t::exec(stmt_p stmt)
{
    for ( ; stmt; stmt = stmt->next) {
        stmt->process(*this);
    }
}

void machine_t::load(decl_p decl)
{
    for (; decl; decl = decl->next)
        decl->process(*this);
}

void machine_t::execute()
{
    for (stmt_p stmt : stmts)
        exec(stmt);
}

number_t machine_t::call(const func_call_t& call)
{
    // evaluate arguments:
    number_t argv[MAX_ARGS];
    int argc = eval_args(argv, call.args);

    // find function:
    auto user_defined_func = funcs.find(call.name);
    if (user_defined_func != funcs.end()) {
        return this->call(*user_defined_func->second, argc, argv);
    }
    
    // find native method:
    auto native_method = native_methods.find(call.name);
    if (native_method != native_methods.end()) {
        return native_method->second.func(native_method->second.context, argc, argv);
    }

    throw undef_func_error{ call.name };
}

number_t machine_t::call(const func_defn_t& func, int argc, number_t argv[MAX_ARGS])
{
    debug << func.name << "()" << std::endl;

    // check args:
    int required_argc = 0;
    for (args_p arg = func.args; arg; arg = arg->next)
        required_argc++;
    if (argc != required_argc) {
        std::cerr << "Wrong number of arguments at func '" << func.name << "': " 
            << required_argc << " required, " << argc << " provided" << std::endl;
        throw wrong_argc_error{ func.name, required_argc, argc };
    }

    // fill local scope:
    var_scope_t local_scope;
    for (args_p arg = func.args; arg; arg = arg->next)
        local_scope.emplace(arg->name, *argv++);

    try {
        var_scope_t *prev_scope = scope;
        auto unwind = on_exit_scope([this, prev_scope] {
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

    // remember local variables so they can be referenced further:
    for (auto& pair : local_scope) {
        auto var = global_scope.find(pair.first);
        if (var == global_scope.end())
            memory_scope.emplace(pair.first, std::move(pair.second));
        else
            var->second = std::move(pair.second);
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
    stmts.push_back(node.stmt);
}

void machine_t::process(const infer_decl_t& node)
{
    infers[node.name] = node.expr;
}

void machine_t::process(const func_defn_t& node)
{
    funcs[node.name] = &node;
}

void machine_t::process(const func_decl_t& node)
{
    // find corresponding object:
    auto obj = native_objects.find(node.object);
    if (obj == native_objects.end())
        throw undef_object_error{ node.object };

    // find corresponding function:
    auto context = obj->second.context;
    auto exports = obj->second.exports;
    native_func_t<void> func = nullptr;
    
    for (auto rec = exports; rec->name; rec++) {
        if (0 == strcmp(rec->name, node.method)) {
            func = rec->func;
            break;
        }
    }

    if (func == nullptr)
        throw undef_method_error{ node.object, node.method };

    native_methods.emplace(node.name, native_method_t{ context, func });
}

NAMESPACE_UBSP_END;