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

machine_t::machine_t(syntax_t& s): 
    syntax(s)
{
    func_scope = &global_scope;
    scope_infers = nullptr;
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

int machine_t::eval_args(number_t argv[MAX_ARGS], const func_call_t& call)
{
    auto unwind = remember(scope_infers);
    auto i = scoped_infers.find(call.name);
    if (i != scoped_infers.end())
        scope_infers = &i->second;
    return eval_args(argv, call.args);
}

array_t& machine_t::find(name_t name)
{
    if (infer_scope) {
        auto it = infer_scope->find(name);
        if (it != infer_scope->end())
            return it->second;
    }

    auto it = func_scope->find(name);
    if (it != func_scope->end())
        return it->second;

    it = global_scope.find(name);
    if (it != global_scope.end())
        return it->second;

    auto global = syntax.get_variable(name);

    if (global && global->infer) {
        var_scope_t scope;
        auto prev_scope = remember(infer_scope);
        infer_scope = &scope;
        exec(global->infer);

        it = global_scope.find(name);
        if (it == global_scope.end())
            throw infer_var_error{ name };
        return it->second;
    }

    throw undef_var_error{ name };
}

number_t machine_t::get(const lvalue_t& lval)
{
    if (scope_infers) {
        auto i = scope_infers->find(lval.name);
        if (i != scope_infers->end()) {
            exec(i->second);
        }
    }

    array_t& value = find(lval.name);

    number_t index[MAX_ARGS];
    int ndims = eval_args(index, lval.index);

    return value.get(ndims, index);
}

void machine_t::put(const lvalue_t& lval, number_t n)
{
    //debug << "    " << lval << " = " << n << std::endl;

    number_t index[MAX_ARGS];
    int ndims = eval_args(index, lval.index);

    auto global = syntax.get_variable(lval.name);
    auto& scope = global ? global_scope : 
        infer_scope ? *infer_scope : *func_scope;

    auto it = scope.find(lval.name);
    if (it == scope.end()) {
        scope.emplace(lval.name, array_t(ndims, index, n));
    }
    else {
        it->second.put(ndims, index, n);
    }
}

number_t machine_t::eval(expr_p expr, number_t default_value)
{
    if (!expr) return default_value;
    expr->process(*this);

    //debug << "        " << expr << " -> " << value << std::endl;

    return value;
}

void machine_t::exec(stmt_p stmt)
{
    for ( ; stmt; stmt = stmt->next) {
        stmt->process(*this);
    }
}

void machine_t::execute()
{
    // collect declarations
    syntax.get_tree_root()->process(*this);

    // execute collected global statements
    for (stmt_p stmt : stmts)
        exec(stmt);
}

number_t machine_t::call(const func_call_t& call)
{
    // evaluate arguments:
    number_t argv[MAX_ARGS];
    int argc = eval_args(argv, call);

    // find native method:
    auto native_method = native_methods.find(call.name);
    if (native_method != native_methods.end()) {
        try {
            value = native_method->second.func(native_method->second.context, argc, argv);
        }
        catch (wrong_argc_error e) {
            e.func = call.name;
            throw e;
        }
    }

    // find function:
    auto user_defined_func = syntax.get_function(call.name);
    if (user_defined_func) {
        return this->call(*user_defined_func, argc, argv);
    }

    if (native_method != native_methods.end())
        return value;

    throw undef_func_error{ call.name };
}

number_t machine_t::call(const function_info_t& func, int argc, number_t argv[MAX_ARGS])
{
    debug << func.name << "()" << std::endl;

    // check args:
    int required_argc = (int) func.args.size();
    if (argc != required_argc) {
        std::cerr << "Wrong number of arguments at func '" << func.name << "': " 
            << required_argc << " required, " << argc << " provided" << std::endl;
        throw wrong_argc_error{ func.name, required_argc, argc };
    }

    // fill local func_scope:
    var_scope_t local_scope;
    for (name_t arg : func.args)
        local_scope.emplace(arg, *argv++);

    try {
        auto unwind = remember(func_scope);
        func_scope = &local_scope;
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

    if (node.type == binary_oper_type_t::AND && left == 0)
        return;

    if (node.type == binary_oper_type_t::OR && left != 0)
        return;

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
    for (exec(node.init); eval(node.cond, 1); exec(node.incr)) {
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
    auto rollback = remember(scope_infers);
    auto i = scoped_infers.find(node.lval.name);
    if (i != scoped_infers.end())
        scope_infers = &i->second;
    number_t value = call(node.call);
    debug << "    " << node.lval << " = " << value << std::endl;
    put(node.lval, value);
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

void machine_t::process(const infer_defn_t& node)
{
    if (node.scope != nullptr) {
        auto i = scoped_infers.find(node.scope);
        if (i == scoped_infers.end())
            i = scoped_infers.emplace(node.scope, infer_map_t()).first;
        i->second[node.name] = node.body;
    }
}

void machine_t::process(const import_decl_t& node)
{
    // find corresponding module:
    auto obj = native_objects.find(node.name);
    if (obj != native_objects.end()) {
        // import module functions:
        auto context = obj->second.context;
        auto exports = obj->second.exports;

        for (auto rec = exports; rec->name; rec++) {
            native_methods.emplace(rec->name, native_method_t{ context, rec->func });
        }
        return;
    }

    syntax.load(node);

    // collect declarations
    node.root.process(*this);
}

NAMESPACE_UBSP_END;