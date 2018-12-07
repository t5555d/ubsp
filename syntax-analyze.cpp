#include "syntax-analyze.h"
#include "syntax.h"
#include "utils.h"

#include <iostream>

NAMESPACE_UBSP_BEGIN;

void syntax_t::analyze()
{
    syntax_analyzer_t analyzer(*this);
    analyzer.analyze();
}

syntax_analyzer_t::syntax_analyzer_t(syntax_t& s) :
    syntax(s), functions(s.function_info), variables(s.variable_info)
{
}

void syntax_analyzer_t::analyze()
{
    syntax.get_tree_root()->process(*this);

    // 1. identify global variables

    for (name_t var : syntax.global_scope.writes) {
        add_global(var);
    }

    for (auto& value : functions) {
        function_info_t& func = value.second;

        for (name_t var : func.loads) {
            add_global(var);
        }

        for (name_t var : func.reads) {
            if (func.writes.find(var) != func.writes.end())
                continue;
            if (std::find(func.args.begin(), func.args.end(), var) != func.args.end())
                continue;
            //std::cout << func.name << " : " << var << std::endl;
            add_global(var);
        }
    }

    // 2. identify owner-functions

    for (auto& value : functions) {
        function_info_t& func = value.second;

        for (name_t var : func.writes) {
            auto it = variables.find(var);
            if (it == variables.end())
                continue;
            auto& info = it->second;
            if (info.func)
                throw dup_var_write_error{ info.name, info.func, func.name };
            info.func = func.name;
            func.globals.insert(var);
        }
    }
}

void syntax_analyzer_t::print_variables(std::ostream& out)
{
    for (auto& value : variables) {
        variable_info_t& var = value.second;
        out << var.name << ": " << (var.func ? var.func : "(global)") << std::endl;
    }
}

//
// declarations
//

void syntax_analyzer_t::process(const root_node_t& root)
{
    for (auto node = root.next; node; node = node->next) {
        node->process(*this);
    }
}

void syntax_analyzer_t::process(const import_decl_t& node)
{
    try {
        syntax.load(node.name);
        process(node.root);
    }
    catch (undef_module_error) {
        syntax.missing_modules.insert(node.name);
    }
}

void syntax_analyzer_t::process(const stmt_decl_t& node)
{
    syntax.global_body.push_back(node.stmt);

    auto prev = remember(func);
    func = &syntax.global_scope;
    process_body(node.stmt);
}

void syntax_analyzer_t::process(const func_defn_t& node)
{
    auto i = functions.find(node.name);
    if (i != functions.end())
        throw dup_func_defn_error{ node.name };

    auto& info = functions[node.name] = function_info_t(node);
    auto prev = remember(func);
    func = &info;
    process_body(node.body);
}

void syntax_analyzer_t::process(const infer_defn_t& node)
{
    if (node.scope != nullptr) {
        auto& m = syntax.scoped_infers;
        auto i = m.find(node.scope);
        if (i == m.end())
            i = m.emplace(node.scope, infer_map_t()).first;
        i->second[node.name] = node.body;
        return;
    }

    auto& info = add_global(node.name);
    if (info.infer)
        throw dup_var_infer_error{ node.name };

    info.infer = node.body;
}

//
// expressions
//

void syntax_analyzer_t::process_args(expr_p args)
{
    for (expr_p expr = args; expr; expr = expr->next)
        expr->process(*this);
}

void syntax_analyzer_t::process(const lval_expr_t& node)
{
    process_args(node.lval.index);
    add_read(node.lval.name);
}

void syntax_analyzer_t::process(const call_expr_t& node)
{
    process_args(node.call.args);
}

void syntax_analyzer_t::process(const chng_expr_t& node)
{
    process_args(node.lval.index);
    add_write(node.lval.name);
    node.value->process(*this);
}

void syntax_analyzer_t::process(const incr_expr_t& node)
{
    process_args(node.lval.index);
    add_read(node.lval.name);
    add_write(node.lval.name);
}

void syntax_analyzer_t::process(const unary_oper_t& node)
{
    node.operand->process(*this);
}

void syntax_analyzer_t::process(const binary_oper_t& node)
{
    node.left->process(*this);
    node.right->process(*this);
}

void syntax_analyzer_t::process(const cond_expr_t& node)
{
    node.cond->process(*this);
    node.expr_true->process(*this);
    node.expr_false->process(*this);
}

//
// statements
//

void syntax_analyzer_t::process_body(stmt_p body)
{
    for (stmt_p stmt = body; stmt; stmt = stmt->next)
        stmt->process(*this);
}

void syntax_analyzer_t::process(const return_stmt_t& node)
{
    if (node.value)
        node.value->process(*this);
}

void syntax_analyzer_t::process(const expr_stmt_t& node)
{
    node.expr->process(*this);
}

void syntax_analyzer_t::process(const cond_stmt_t& node)
{
    node.cond->process(*this);
    process_body(node.stmt_true);
    process_body(node.stmt_false);
}

void syntax_analyzer_t::process(const loop_stmt_t& node)
{
    node.cond->process(*this);
    process_body(node.body);
}

void syntax_analyzer_t::process(const for_loop_stmt_t& node)
{
    process_body(node.init);
    process_args(node.cond);
    process_body(node.incr);
    process_body(node.body);
}

void syntax_analyzer_t::process(const load_stmt_t& node)
{
    process_args(node.lval.index);
    add_write(node.lval.name);
    add_load(node.lval.name);
    process_args(node.call.args);
}

//
// common methods
//

void syntax_analyzer_t::add_read(name_t var)
{
    func->reads.insert(var);
}

void syntax_analyzer_t::add_load(name_t var)
{
    func->loads.insert(var);
}

void syntax_analyzer_t::add_write(name_t var)
{
    func->writes.insert(var);
}

variable_info_t& syntax_analyzer_t::add_global(name_t var)
{
    auto info = variables.find(var);
    if (info == variables.end()) {
        info = variables.emplace(var, var).first;
    }
    return info->second;
}

NAMESPACE_UBSP_END;