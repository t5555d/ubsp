#include <iostream>
#include <cassert>
#include "syntax-loader.h"
#include "syntax.tab.hpp"

int yyparse(ubsp::syntax_loader_t *);

NAMESPACE_UBSP_BEGIN;

syntax_loader_t::syntax_loader_t(syntax_t& s, const char *filename) :
    syntax(s), file(nullptr), ungot_char(EOF), line(1), column(1)
{
    file = fopen(filename, "r");
    if (file == nullptr) {
        int code = errno;
        throw std::system_error(code, std::generic_category());
    }
}

syntax_loader_t::~syntax_loader_t()
{
    fclose(file);
}

int syntax_loader_t::parse()
{
    return yyparse(this);
}

int yylex(YYSTYPE *yylval, YYLTYPE *yylloc, syntax_loader_t *runtime)
{
    runtime->update_position(yylloc->first_line, yylloc->first_column);
    int token = runtime->read_token(yylval);
    runtime->update_position(yylloc->last_line, yylloc->last_column);
    return token;
}

void yyerror(const YYLTYPE *yylloc, syntax_loader_t *runtime, const char *msg)
{
    std::cerr << yylloc->last_line << ':' << yylloc->last_column << ": " << msg << std::endl;
}

void syntax_loader_t::update_position(int& row, int& col)
{
    row = line;
    col = column;
}

int syntax_loader_t::getc()
{
    if (ungot_char != EOF) {
        int chr = ungot_char;
        ungot_char = EOF;
        return chr;
    }

    int chr = fgetc(file);
    if (chr == '\n') {
        line++;
        column = 1;
    }
    else if (chr != EOF) {
        column++;
    }
    return chr;
}

void syntax_loader_t::ungetc(int chr)
{
    ungot_char = chr;
}

int syntax_loader_t::check_change(int token)
{
    int next = getc();
    if (next == '=') return CHANGE;
    ungetc(next);
    return token;
}

number_t syntax_loader_t::read_number()
{
    char buf[MAX_IDENT_LEN];
    int i = 0, radix = 10, c = getc();
    if (c != '0') {
        do {
            buf[i++] = c;
            c = getc();
        } while (c >= '0' && c <= '9');
    }
    else {
        c = getc();
        if (c == 'x' || c == 'X') {
            radix = 16;
            c = getc();
            do {
                buf[i++] = c;
                c = getc();
            } while (c >= '0' && c <= '9' || c >= 'A' && c <= 'F' || c >= 'a' && c <= 'f');
        }
        else if (c == 'b' || c == 'B') {
            radix = 2;
            c = getc();
            do {
                buf[i++] = c;
                c = getc();
            } while (c >= '0' && c <= '1');
        }
        else {
            while (c >= '0' && c <= '9') {
                buf[i++] = c;
                c = getc();
            }
        }
    }

    ungetc(c);
    buf[i] = 0;
    return strtoll(buf, nullptr, radix);
}

void syntax_loader_t::skip_single_line_comment()
{
    for (int c = getc(); c != EOF; c = getc()) {
        if (c == '\n')
            break;
    }
}

void syntax_loader_t::skip_multi_line_comment()
{
    for (int c = getc(), n = getc(); c != EOF; c = n, n = getc()) {
        if (c == '*' && n == '/')
            break;
    }
}

constexpr int pair(int first, int second) {
    return first | (second << 8);
}

constexpr int pair(char x[2]) {
    return x[0] | (x[1] << 8);
}

int syntax_loader_t::read_token(YYSTYPE *yylval)
{
    char buf[MAX_IDENT_LEN];
    int bi = 0;
    while (true) {
        int value = getc();

        if (value == EOF)
            return 0;

        if (isspace(value))
            continue;

        if (isdigit(value)) {
            ungetc(value);
            yylval->number = read_number();
            return NUMBER;
        }

        if (isalpha(value)) {
            do {
                buf[bi++] = value;
                value = getc();
            } while (isalpha(value) || isdigit(value) || value == '_');
            ungetc(value);
            buf[bi] = 0;

            // check reserved words:
            if (0 == strcmp(buf, "if"))
                return IF;
            if (0 == strcmp(buf, "else"))
                return ELSE;
            if (0 == strcmp(buf, "do"))
                return DO;
            if (0 == strcmp(buf, "while"))
                return WHILE;
            if (0 == strcmp(buf, "for"))
                return FOR;
            if (0 == strcmp(buf, "return"))
                return RETURN;
            if (0 == strcmp(buf, "break"))
                return BREAK;
            if (0 == strcmp(buf, "continue"))
                return CONTINUE;
            if (0 == strcmp(buf, "global"))
                return GLOBAL;
            if (0 == strcmp(buf, "extern"))
                return EXTERN;

            // save identifier:
            auto i = syntax.idents.emplace(buf);
            yylval->ident = i.first->c_str();
            return IDENT;
        }

        // one-char tokens without yylval:
        if (strchr("{}()[]?,:;.", value)) {
            return value;
        }

        // one-char tokens with yylval:
        int token = value;
        switch (value) {
        case '&': yylval->bt = binary_oper_type_t::BAND; break;
        case '|': yylval->bt = binary_oper_type_t::BOR; break;
        case '^': yylval->bt = binary_oper_type_t::XOR; break;
        case '+': yylval->bt = binary_oper_type_t::ADD; break;
        case '-': yylval->bt = binary_oper_type_t::SUB; break;
        case '*': yylval->bt = binary_oper_type_t::MUL; break;
        case '/': yylval->bt = binary_oper_type_t::DIV; break;
        case '%': yylval->bt = binary_oper_type_t::MOD; break;
        case '=': yylval->bt = binary_oper_type_t::NOP; break;
        case '<': yylval->bt = binary_oper_type_t::LT; break;
        case '>': yylval->bt = binary_oper_type_t::GT; break;
        case '!': yylval->ut = unary_oper_type_t::NOT; break;
        case '~': yylval->ut = unary_oper_type_t::BNOT; break;
        default: token = 0;
        }

        // two-char tokens with yylval:
        if (token > 0) {
            int next = getc();
            switch (pair(value, next)) {
            case pair("&="): case pair("|="): 
            case pair("^="): case pair("+="): 
            case pair("-="): case pair("*="): 
            case pair("/="): case pair("%="): 
                return CHANGE;
            case pair("++"): return INC;
            case pair("--"): return DEC;
            case pair("<<"): yylval->bt = binary_oper_type_t::SHL; return check_change(SHL);
            case pair(">>"): yylval->bt = binary_oper_type_t::SHR; return check_change(SHR);
            case pair("&&"): yylval->bt = binary_oper_type_t::AND; return check_change(AND);
            case pair("||"): yylval->bt = binary_oper_type_t::OR; return check_change(OR);
            case pair("=="): yylval->bt = binary_oper_type_t::EQ; return EQ;
            case pair("!="): yylval->bt = binary_oper_type_t::NE; return NE;
            case pair("<="): yylval->bt = binary_oper_type_t::LE; return LE;
            case pair(">="): yylval->bt = binary_oper_type_t::GE; return GE;
            case pair("//"): skip_single_line_comment(); continue;
            case pair("/*"): skip_multi_line_comment(); continue;
            }
            ungetc(next);

            return token;
        }

        std::cerr << "Unexpected symbol: '" << char(value) << "' (" << value << ")" << std::endl;

        return value;
    }
}

void *syntax_loader_t::alloc_buffer()
{
    node_buffer_t *buffer = syntax.first_free;
    if (buffer) {
        syntax.first_free = buffer->next_free;
    }
    else {
        node_buffer_t empty;
        syntax.pool.emplace_back(empty);
        buffer = &syntax.pool.back();
    }
    return buffer;
}

void syntax_loader_t::free_buffer(void *address)
{
    node_buffer_t *buffer = static_cast<node_buffer_t *>(address);
    buffer->next_free = syntax.first_free;
    syntax.first_free = buffer;
}

#define ARG_DEF(type, name) decltype(type##_t::name) name

#define CREATE_NODE_FUNC_1(type, name0) \
    type##_t *syntax_loader_t::create_##type(ARG_DEF(type, name0)) { \
        type##_t *node = alloc_node<type##_t>(); \
        node->name0 = name0; \
        return node; }

#define CREATE_NODE_FUNC_2(type, name0, name1) \
    type##_t *syntax_loader_t::create_##type(ARG_DEF(type, name0), ARG_DEF(type, name1)) { \
        type##_t *node = alloc_node<type##_t>(); \
        node->name0 = name0; \
        node->name1 = name1; \
        return node; }

#define CREATE_NODE_FUNC_3(type, name0, name1, name2) \
    type##_t *syntax_loader_t::create_##type(ARG_DEF(type, name0), ARG_DEF(type, name1), ARG_DEF(type, name2)) { \
        type##_t *node = alloc_node<type##_t>(); \
        node->name0 = name0; \
        node->name1 = name1; \
        node->name2 = name2; \
        return node; }

CREATE_NODE_FUNC_1(const_expr, value);
CREATE_NODE_FUNC_1(lval_expr, lval);
CREATE_NODE_FUNC_1(call_expr, call);
CREATE_NODE_FUNC_3(chng_expr, lval, value, type);
CREATE_NODE_FUNC_2(incr_expr, type, lval);
CREATE_NODE_FUNC_2(unary_oper, type, operand);
CREATE_NODE_FUNC_3(binary_oper, type, left, right);
CREATE_NODE_FUNC_3(cond_expr, cond, expr_true, expr_false);

CREATE_NODE_FUNC_2(load_stmt, lval, call);
CREATE_NODE_FUNC_1(return_stmt, value);
CREATE_NODE_FUNC_1(expr_stmt, expr);
CREATE_NODE_FUNC_3(cond_stmt, cond, stmt_true, stmt_false);
CREATE_NODE_FUNC_3(loop_stmt, cond, body, pre_check);
CREATE_NODE_FUNC_3(for_loop_stmt, cond, body, incr);

CREATE_NODE_FUNC_1(stmt_decl, stmt);
CREATE_NODE_FUNC_3(func_defn, name, args, body);
CREATE_NODE_FUNC_3(func_decl, name, object, method);

CREATE_NODE_FUNC_1(argument, name);

incr_expr_t *syntax_loader_t::create_incr_expr(lvalue_t lval, binary_oper_type_t type)
{
    auto incr = create_incr_expr(type, lval);
    incr->postfix = true;
    return incr;
}

void syntax_loader_t::register_func(name_t name, args_p args, stmt_p body)
{
    auto decl = create_func_defn(name, args, body);
    register_decl(decl);
}

void syntax_loader_t::register_func(name_t name, name_t object, name_t method)
{
    auto decl = create_func_decl(name, object, method);
    register_decl(decl);
}

void syntax_loader_t::register_stmt(stmt_p stmt)
{
    auto decl = create_stmt_decl(stmt);
    register_decl(decl);
}

void syntax_loader_t::register_decl(decl_p decl)
{
    chain(syntax.last, decl);
    syntax.last = decl;
}

NAMESPACE_UBSP_END;