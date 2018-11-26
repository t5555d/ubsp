#ifndef _UBSP_SYNTAX_ANALYZE_
#define _UBSP_SYNTAX_ANALYZE_

#include "ubsp-fwd.h"
#include <list>
#include <map>
#include <set>

NAMESPACE_UBSP_BEGIN;

class syntax_analyzer_t : private syntax_processor_i
{
public:
    syntax_analyzer_t(syntax_t& s);

    void analyze();

private:
    virtual void process(const const_expr_t&) override {}
    virtual void process(const lval_expr_t&) override;
    virtual void process(const call_expr_t&) override;
    virtual void process(const chng_expr_t&) override;
    virtual void process(const incr_expr_t&) override;
    virtual void process(const unary_oper_t&) override;
    virtual void process(const binary_oper_t&) override;
    virtual void process(const cond_expr_t&) override;
    virtual void process(const break_stmt_t&) override {}
    virtual void process(const continue_stmt_t&) override {}
    virtual void process(const return_stmt_t&) override;
    virtual void process(const expr_stmt_t&) override;
    virtual void process(const cond_stmt_t&) override;
    virtual void process(const loop_stmt_t&) override;
    virtual void process(const for_loop_stmt_t&) override;
    virtual void process(const load_stmt_t&) override;

    virtual void process(const root_node_t&) override;
    virtual void process(const stmt_decl_t&) override;
    virtual void process(const func_defn_t&) override;
    virtual void process(const import_decl_t&) override;
    virtual void process(const infer_defn_t&) override;

private:
    syntax_t& syntax;
    std::map<name_t, function_info_t>& functions;
    std::map<name_t, variable_info_t>& variables;

    function_info_t *func = nullptr;

    void process_args(expr_p args);
    void process_body(stmt_p body);

    void add_read(name_t var_name);
    void add_load(name_t var_name);
    void add_write(name_t var_name);
    variable_info_t& add_global(name_t var_name);
};

struct dup_func_defn_error {
    name_t func;
};

struct dup_var_infer_error {
    name_t var;
};

struct dup_var_write_error {
    name_t var;
    name_t func1;
    name_t func2;
};

NAMESPACE_UBSP_END;

#endif // _UBSP_SYNTAX_ANALYZE_