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
    expr->process(this);
    if (inner_priority < outer_priority) out << ")";
    this->priority = outer_priority;
    return *this;
}

syntax_output_t& syntax_output_t::operator<<(stmt_p stmt)
{
    if (stmt) 
        stmt->process(this);
    return *this;
}

const char *to_string(scope_exit_type_t type)
{
    static const char *strings[] = {
        "return",   // RETURN
        "break",    // BREAK
        "continue", // CONTINUE
    };
    static_assert(sizeof(strings) / sizeof(strings[0]) == NUM_SCOPE_EXIT_TYPES, "All values should be printed");
    return strings[int(type)];
}


               
//
// output interface implementation:
//

// expressions:

void syntax_output_t::process(const const_expr_t *value) { out << value->value; }
void syntax_output_t::process(const lval_expr_t *value) { *this << value->lval; }
void syntax_output_t::process(const call_expr_t *value) { *this << value->call; }

void syntax_output_t::process(const chng_expr_t *value)
{
    auto& info = get_operator_info(value->type);
    *this << value->lval << " " << info.text << "= " << value->value;
}

void syntax_output_t::process(const incr_expr_t *value)
{
    auto& info = get_operator_info(value->type);
    if (value->postfix)
        *this << value->lval << info.text << info.text;
    else
        *this << info.text << info.text << value->lval;
}

void syntax_output_t::process(const unary_oper_t *value)
{
    auto& info = get_operator_info(value->type);
    *this << info.text << value->operand;
}

void syntax_output_t::process(const binary_oper_t *value)
{
    auto& info = get_operator_info(value->type);
    *this << value->left << " " << info.text << " ";
    priority++; // left associativity
    *this << value->right;
}

void syntax_output_t::process(const cond_expr_t *value)
{
    priority++;
    *this << value->cond << " ? " << value->expr_true;
    priority--;
    *this << " : " << value->expr_false;
}

// statements:

void syntax_output_t::process(const root_stmt_t *value)
{
    for (stmt_p stmt = value->next; stmt; stmt = stmt->next) {
        stmt->process(this);
        out << std::endl;
    }
}

void syntax_output_t::process(const exit_stmt_t *value)
{
    *this << to_string(value->type) << " " << value->value;
}

void syntax_output_t::process(const expr_stmt_t *value)
{
    *this << value->expr;
}

void syntax_output_t::output_block(stmt_p block)
{
    const char *indent_text = "    ";
    out << "{" << std::endl;
    indent++;
    for (stmt_p stmt = block; stmt; stmt = stmt->next) {
        for (int i = 0; i < indent; i++)
            out << "    ";
        stmt->process(this);
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

void syntax_output_t::process(const cond_stmt_t *value)
{
    *this << "if (" << value->cond << ") ";
    output_block(value->stmt_true);
    if (value->stmt_false) {
        *this << "else ";
        output_block(value->stmt_false);
    }
}

void syntax_output_t::process(const loop_stmt_t *value)
{
    if (value->pre_check) {
        *this << "while (" << value->cond << ") ";
        output_block(value->body);
    }
    else {
        *this << "do ";
        output_block(value->body);
        *this << "while (" << value->cond << ")";
    }
}

void syntax_output_t::process(const for_loop_stmt_t *value)
{
    *this << "for (; " << value->cond << "; ";
    output_inits(value->incr);
    *this << ") ";
    output_block(value->body);
}

void syntax_output_t::process(const load_stmt_t *value)
{
    *this << value->lval << " " << value->call;
}

void syntax_output_t::process(const func_defn_t *value)
{
    *this << value->name << "()";
    output_block(value->body);
}


NAMESPACE_UBSP_END;