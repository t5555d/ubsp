#ifndef _UBSP_SYNTAX_OUTPUT_
#define _UBSP_SYNTAX_OUTPUT_

#include "ubsp-fwd.h"
#include "operators.h"
#include <ostream>

NAMESPACE_UBSP_BEGIN;

class syntax_output_t : public syntax_processor_i
{
public:
    syntax_output_t(std::ostream& o) : out(o) {}

    virtual void process(const const_expr_t&) override;
    virtual void process(const lval_expr_t&) override;
    virtual void process(const call_expr_t&) override;
    virtual void process(const chng_expr_t&) override;
    virtual void process(const incr_expr_t&) override;
    virtual void process(const unary_oper_t&) override;
    virtual void process(const binary_oper_t&) override;
    virtual void process(const cond_expr_t&) override;
    virtual void process(const break_stmt_t&) override;
    virtual void process(const continue_stmt_t&) override;
    virtual void process(const return_stmt_t&) override;
    virtual void process(const expr_stmt_t&) override;
    virtual void process(const cond_stmt_t&) override;
    virtual void process(const loop_stmt_t&) override;
    virtual void process(const for_loop_stmt_t&) override;
    virtual void process(const load_stmt_t&) override;
    virtual void process(const root_node_t&) override;
    virtual void process(const stmt_decl_t&) override;
    virtual void process(const func_defn_t&) override;
    virtual void process(const func_decl_t&) override;
    virtual void process(const infer_decl_t&) override;

    syntax_output_t& operator<<(const char *s) { out << s; return *this; }
    syntax_output_t& operator<<(number_t n) { out << n; return *this; }
    syntax_output_t& operator<<(const lvalue_t& lval);
    syntax_output_t& operator<<(const func_call_t& call);
    syntax_output_t& operator<<(std::ostream& (*fn)(std::ostream&)) { // support std::endl
        fn(out);
        return *this;
    }

    syntax_output_t& operator<<(expr_p expr);
    syntax_output_t& operator<<(stmt_p stmt);
    syntax_output_t& operator<<(args_p args);

private:
    std::ostream& out;
    int indent = 0;
    int priority = NOP_PRIORITY;

    void output_block(stmt_p block);
    void output_inits(stmt_p inits);
};

NAMESPACE_UBSP_END;

#endif // _UBSP_SYNTAX_OUTPUT_