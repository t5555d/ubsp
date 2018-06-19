#ifndef _UBSP_MACHINE_
#define _UBSP_MACHINE_

#include "ubsp-fwd.h"
#include <map>

NAMESPACE_UBSP_BEGIN;

typedef std::map<name_t, number_t> var_scope_t;

class machine_t : private syntax_processor_i
{
public:

    machine_t();

    number_t call(const func_call_t& call);
    number_t eval(expr_p expr, number_t default_value = 0);
    void exec(stmt_p stmt);

    number_t get(const lvalue_t& lval);
    void put(const lvalue_t& lval, number_t n);

private:
    std::map<name_t, const func_defn_t *> funcs;
    var_scope_t global_scope, *scope;
    number_t value;

    virtual void process(const const_expr_t&) override;
    virtual void process(const lval_expr_t&) override;
    virtual void process(const call_expr_t&) override;
    virtual void process(const chng_expr_t&) override;
    virtual void process(const incr_expr_t&) override;
    virtual void process(const unary_oper_t&) override;
    virtual void process(const binary_oper_t&) override;
    virtual void process(const cond_expr_t&) override;
    virtual void process(const root_stmt_t&) override;
    virtual void process(const exit_stmt_t&) override;
    virtual void process(const expr_stmt_t&) override;
    virtual void process(const cond_stmt_t&) override;
    virtual void process(const loop_stmt_t&) override;
    virtual void process(const for_loop_stmt_t&) override;
    virtual void process(const load_stmt_t&) override;
    virtual void process(const func_defn_t&) override;
};


NAMESPACE_UBSP_END;

#endif // _UBSP_MACHINE_