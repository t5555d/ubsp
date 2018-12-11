#include "machine.h"
#include "syntax.h"
#include "operators.h"
#include "utils.h"
#include "error.h"
#include <iostream>

#include "syntax-output.h"

NAMESPACE_UBSP_BEGIN;

struct break_exception {};
struct continue_exception {};
struct return_exception { number_t value; };

syntax_output_t debug(std::cerr);

machine_t::machine_t(const syntax_t& s):
    syntax(s)
{
    func_scope = &global_scope;
    scope_infers = nullptr;
}

array_t *machine_t::scope_t::find(name_t name)
{
    auto it = vars.find(name);
    return it != vars.end() ? &it->second : nullptr;
}

const char *INDENT = "    ";

void machine_t::scope_t::print_open() const
{
    if (output) return;
    if (prev) prev->print_open();
    for (int i = 0; i < indent; i++) debug << INDENT;
    debug << "<" << name << ">" << std::endl;
    output = true;
}

void machine_t::scope_t::print_close() const
{
    if (!output) return;
    for (int i = 0; i < indent; i++) debug << INDENT;
    debug << "</" << name << ">" << std::endl;
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
    auto i = syntax.scoped_infers.find(call.name);
    if (i != syntax.scoped_infers.end())
        scope_infers = &i->second;
    return eval_args(argv, call.args);
}

array_t& machine_t::find(name_t name)
{
    array_t *var = nullptr;
    if (infer_scope) {
        var = infer_scope->find(name);
        if (var) return *var;
    }

    var = func_scope->find(name);
    if (var) return *var;

    var = global_scope.find(name);
    if (var) return *var;

    auto global = syntax.get_variable(name);

    if (global && global->infer) {
        scope_t scope(name, func_scope);
        auto prev_scope = remember(infer_scope);
        infer_scope = &scope;
        exec(global->infer);

        var = global_scope.find(name);
        if (var) return *var;
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

    if (ndims != value.num_dimensions())
        throw wrong_ndims_error{ lval.name, ndims, value.num_dimensions() };

    try {
        return value.get(ndims, index);
    }
    catch (wrong_index_error& e) {
        e.name = lval.name;
        throw e;
    }
    catch (array_not_init_error& e) {
        e.name = lval.name;
        throw e;
    }
}

void machine_t::put(const lvalue_t& lval, number_t n, bool load)
{
    number_t index[MAX_ARGS];
    int ndims = eval_args(index, lval.index);

    auto global = syntax.get_variable(lval.name);
    if (global && global->is_const()) {
        if (ndims != 0)
            throw const_ndims_error{ lval.name, ndims };
        if (n != global->const_value)
            throw const_value_error{ lval.name, n, global->const_value };
        return;
    }

    if (load) {
        auto& scope = infer_scope ? *infer_scope :
            func_scope ? *func_scope : global_scope;
        scope.print_open();
        for (int i = 0; i <= scope.indent; i++)
            debug << INDENT;
        debug << "<" << lval.name;
        if (ndims) {
            debug << " index=\"";
            for (int i = 0; i < ndims; i++)
                debug << index[i] << " ";
            debug << "\"";
        }
        debug << " value=\"" << n << "\"/>" << std::endl;
    }

    auto& scope = global ? global_scope :
        infer_scope ? *infer_scope : *func_scope;

    auto var = scope.find(lval.name);
    if (var == nullptr)
        scope.vars.emplace(lval.name, array_t(ndims, index, n));
    else if (ndims != var->num_dimensions())
        throw wrong_ndims_error{ lval.name, ndims, var->num_dimensions() };
    else
        var->put(ndims, index, n);
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

void machine_t::execute()
{
    // import native modules
    for (name_t module : syntax.missing_modules) {
        auto obj = native_modules.find(module);
        if (obj == native_modules.end())
            throw undef_module_error{ module };

        // import module functions:
        auto context = obj->second.context;
        auto exports = obj->second.exports;

        for (auto rec = exports; rec->name; rec++) {
            native_methods.emplace(rec->name, native_method_t{ context, rec->func });
        }
    }

    // execute global statements
    for (stmt_p stmt : syntax.global_body)
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
    // check args:
    int required_argc = (int) func.args.size();
    if (argc != required_argc) {
        std::cerr << "Wrong number of arguments at func '" << func.name << "': " 
            << required_argc << " required, " << argc << " provided" << std::endl;
        throw wrong_argc_error{ func.name, required_argc, argc };
    }

    // fill local func_scope:
    scope_t local_scope(func.name, func_scope);
    for (name_t arg : func.args)
        local_scope.vars.emplace(arg, *argv++);

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
    auto i = syntax.scoped_infers.find(node.lval.name);
    if (i != syntax.scoped_infers.end())
        scope_infers = &i->second;
    number_t value = call(node.call);

    put(node.lval, value, true);
}

NAMESPACE_UBSP_END;