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

    virtual void process(const const_expr_t *) override;
    virtual void process(const lval_expr_t *) override;
    virtual void process(const call_expr_t *) override;
    virtual void process(const chng_expr_t *) override;
    virtual void process(const incr_expr_t *) override;
    virtual void process(const unary_oper_t *) override;
    virtual void process(const binary_oper_t *) override;
    virtual void process(const cond_expr_t *) override;
    virtual void process(const root_stmt_t *) override;
    virtual void process(const exit_stmt_t *) override;
    virtual void process(const expr_stmt_t *) override;
    virtual void process(const cond_stmt_t *) override;
    virtual void process(const loop_stmt_t *) override;
    virtual void process(const for_loop_stmt_t *) override;
    virtual void process(const load_stmt_t *) override;
    virtual void process(const func_defn_t *) override;

private:
    std::ostream& out;
    int indent = 0;

    void output_block(stmt_p block);
    void output_inits(stmt_p inits);
};

// helper functions:
inline const char *to_string(binary_oper_type_t type) { return get_operator_info(type).text; }
inline const char *to_string(unary_oper_type_t type) { return get_operator_info(type).text; }
const char *to_string(scope_exit_type_t type);

inline std::ostream& operator<<(std::ostream& out, binary_oper_type_t type) { return out << to_string(type); }
inline std::ostream& operator<<(std::ostream& out, unary_oper_type_t type) { return out << to_string(type); }
inline std::ostream& operator<<(std::ostream& out, scope_exit_type_t type) { return out << to_string(type); }

std::ostream& operator<<(std::ostream& out, const syntax_node_i *node);
std::ostream& operator<<(std::ostream& out, const lvalue_t& lval);
std::ostream& operator<<(std::ostream& out, const func_call_t& call);


NAMESPACE_UBSP_END;

#endif // _UBSP_SYNTAX_OUTPUT_