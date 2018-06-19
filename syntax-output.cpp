#include "syntax-output.h"
#include "syntax.h"

NAMESPACE_UBSP_BEGIN;

//
// helper functions:
//

syntax_output_t& syntax_output_t::operator<<(const lvalue_t& lval)
{
    *this << lval.name;
    this->priority = NOP_PRIORITY;
    for (expr_p idx = lval.index; idx; idx = idx->next)
        *this << "[" << idx << "]";
    return *this;
}

syntax_output_t& syntax_output_t::operator<<(const func_call_t& call)
{
    *this << call.name << "(";
    this->priority = NOP_PRIORITY;
    if (expr_p arg = call.args) {
        *this << arg;
        while ((arg = arg->next) != nullptr)
            *this << ", " << arg;
    }
    return *this << ")";
}

syntax_output_t& syntax_output_t::operator<<(expr_p expr)
{
    if (expr == nullptr) return *this;
    auto outer_priority = this->priority;
    auto inner_priority = expr->get_priority();
    this->priority = inner_priority;
    if (inner_priority < outer_priority) out << "(";
    expr->process(*this);
    if (inner_priority < outer_priority) out << ")";
    this->priority = outer_priority;
    return *this;
}

syntax_output_t& syntax_output_t::operator<<(stmt_p stmt)
{
    if (stmt) 
        stmt->process(*this);
    return *this;
}
               
//
// output interface implementation:
//

// expressions:

void syntax_output_t::process(const const_expr_t& node) { out << node.value; }
void syntax_output_t::process(const lval_expr_t& node) { *this << node.lval; }
void syntax_output_t::process(const call_expr_t& node) { *this << node.call; }

void syntax_output_t::process(const chng_expr_t& node)
{
    auto& info = get_operator_info(node.type);
    *this << node.lval << " " << info.text << "= " << node.value;
}

void syntax_output_t::process(const incr_expr_t& node)
{
    auto& info = get_operator_info(node.type);
    if (node.postfix)
        *this << node.lval << info.text << info.text;
    else
        *this << info.text << info.text << node.lval;
}

void syntax_output_t::process(const unary_oper_t& node)
{
    auto& info = get_operator_info(node.type);
    *this << info.text << node.operand;
}

void syntax_output_t::process(const binary_oper_t& node)
{
    auto& info = get_operator_info(node.type);
    *this << node.left << " " << info.text << " ";
    priority++; // left associativity
    *this << node.right;
}

void syntax_output_t::process(const cond_expr_t& node)
{
    priority++;
    *this << node.cond << " ? " << node.expr_true;
    priority--;
    *this << " : " << node.expr_false;
}

// statements:

void syntax_output_t::process(const root_stmt_t& node)
{
    for (stmt_p stmt = node.next; stmt; stmt = stmt->next) {
        stmt->process(*this);
        out << std::endl;
    }
}

void syntax_output_t::process(const break_stmt_t& node) { *this << "break"; }
void syntax_output_t::process(const continue_stmt_t& node) { *this << "continue"; }
void syntax_output_t::process(const return_stmt_t& node) { *this << "return " << node.value; }
void syntax_output_t::process(const expr_stmt_t& node) { *this << node.expr; }

void syntax_output_t::output_block(stmt_p block)
{
    const char *indent_text = "    ";
    out << "{" << std::endl;
    indent++;
    for (stmt_p stmt = block; stmt; stmt = stmt->next) {
        for (int i = 0; i < indent; i++)
            out << "    ";
        stmt->process(*this);
        out << std::endl;
    }
    indent--;
    for (int i = 0; i < indent; i++)
        out << "    ";
    out << "}";
}

void syntax_output_t::output_inits(stmt_p inits)
{
    if (stmt_p init = inits) {
        *this << init;
        while ((init = init->next) != nullptr)
            *this << ", " << init;
    }
}

void syntax_output_t::process(const cond_stmt_t& node)
{
    *this << "if (" << node.cond << ") ";
    output_block(node.stmt_true);
    if (node.stmt_false) {
        *this << "else ";
        output_block(node.stmt_false);
    }
}

void syntax_output_t::process(const loop_stmt_t& node)
{
    if (node.pre_check) {
        *this << "while (" << node.cond << ") ";
        output_block(node.body);
    }
    else {
        *this << "do ";
        output_block(node.body);
        *this << "while (" << node.cond << ")";
    }
}

void syntax_output_t::process(const for_loop_stmt_t& node)
{
    *this << "for (; " << node.cond << "; ";
    output_inits(node.incr);
    *this << ") ";
    output_block(node.body);
}

void syntax_output_t::process(const load_stmt_t& node)
{
    *this << node.lval << " " << node.call;
}

void syntax_output_t::process(const func_defn_t& node)
{
    *this << node.name << "() ";
    output_block(node.body);
}


NAMESPACE_UBSP_END;