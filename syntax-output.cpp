#include "syntax-output.h"
#include "syntax.h"

NAMESPACE_UBSP_BEGIN;

//
// helper functions:
//

std::ostream& operator<<(std::ostream& out, const syntax_node_i *node)
{
    if (node) {
        syntax_output_t output(out);
        node->process(&output);
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, const lvalue_t& lval)
{
    out << lval.name;
    for (expr_p idx = lval.index; idx; idx = idx->next)
        out << '[' << idx << ']';
    return out;
}

std::ostream& operator<<(std::ostream& out, const func_call_t& call)
{
    out << call.name << '(';
    if (expr_p arg = call.args) {
        out << arg;
        while ((arg = arg->next) != nullptr)
            out << ", " << arg;
    }
    return out << ')';
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
void syntax_output_t::process(const lval_expr_t *value) { out << value->lval; }
void syntax_output_t::process(const call_expr_t *value) { out << value->call; }

void syntax_output_t::process(const chng_expr_t *value)
{
    out << value->lval << ' ' << value->type << "= " << value->value;
}

void syntax_output_t::process(const incr_expr_t *value)
{
    if (value->postfix)
        out << value->lval << value->type << value->type;
    else
        out << value->type << value->type << value->lval;
}

void syntax_output_t::process(const unary_oper_t *value)
{
    out << value->type << value->operand;
}

void syntax_output_t::process(const binary_oper_t *value)
{
    if (value->left->get_priority() < value->get_priority())
        out << '(' << value->left << ')';
    else
        out << value->left;

    out << ' ' << value->type << ' ';

    if (value->right->get_priority() < value->get_priority())
        out << '(' << value->right << ')';
    else
        out << value->right;
}

void syntax_output_t::process(const cond_expr_t *value)
{
    out << value->cond << " ? " << value->expr_true << " : " << value->expr_false;
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
    out << value->type << ' ' << value->value;
}

void syntax_output_t::process(const expr_stmt_t *value)
{
    out << value->expr;
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
        out << init;
        while ((init = init->next) != nullptr)
            out << ", " << init;
    }
}

void syntax_output_t::process(const cond_stmt_t *value)
{
    out << "if (" << value->cond << ") ";
    output_block(value->stmt_true);
    if (value->stmt_false) {
        out << "else ";
        output_block(value->stmt_false);
    }
}

void syntax_output_t::process(const loop_stmt_t *value)
{
    if (value->pre_check) {
        out << "while (" << value->cond << ") ";
        output_block(value->body);
    }
    else {
        out << "do ";
        output_block(value->body);
        out << "while (" << value->cond << ")";
    }
}

void syntax_output_t::process(const for_loop_stmt_t *value)
{
    out << "for (; " << value->cond << "; ";
    output_inits(value->incr);
    out << ") ";
    output_block(value->body);
}

void syntax_output_t::process(const load_stmt_t *value)
{
    out << value->lval << " " << value->call;
}

void syntax_output_t::process(const func_defn_t *value)
{
    out << value->name << "()";
    output_block(value->body);
}


NAMESPACE_UBSP_END;