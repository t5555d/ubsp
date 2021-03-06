#ifndef _UBSP_SYNTAX_SCANNER_
#define _UBSP_SYNTAX_SCANNER_

#include "syntax.h"
typedef union YYSTYPE YYSTYPE;
typedef struct YYLTYPE YYLTYPE;

int yyparse(ubsp::syntax_loader_t *);

NAMESPACE_UBSP_BEGIN;

//
// syntax loader:
//

class syntax_loader_t
{
public:
    syntax_loader_t(syntax_t& s, FILE *f, decl_p root);
    ~syntax_loader_t();

    int parse();

private:
    syntax_t& syntax;
    decl_p last;
    FILE *file;
    int ungot_char;
    int line, column;

    int getc();
    void ungetc(int chr);
    int check_change(int token);

    void update_position(int& line, int& column);

    int read_token(YYSTYPE *);
    number_t read_number();
    void skip_single_line_comment();
    void skip_multi_line_comment();

    friend int yylex(YYSTYPE *yylval, YYLTYPE *yylloc, syntax_loader_t *runtime);
    friend void yyerror(const YYLTYPE *yylloc, syntax_loader_t *runtime, const char *msg);

    template<typename T>
    T *alloc_node() {
        static_assert(sizeof(T) <= MAX_NODE_SIZE, "buffer size is not enough");
        void *buffer = alloc_buffer();
        return new (buffer) T; // default constructor required
    }
    void *alloc_buffer();
    void free_buffer(void *address);

    friend int ::yyparse(ubsp::syntax_loader_t *);

    const_expr_t    *create_const_expr(number_t value);
    lval_expr_t     *create_lval_expr(lvalue_t lval);
    call_expr_t     *create_call_expr(func_call_t call);
    chng_expr_t     *create_chng_expr(lvalue_t lval, expr_p value, binary_oper_type_t type);
    incr_expr_t     *create_incr_expr(binary_oper_type_t type, lvalue_t lval); // prefix
    incr_expr_t     *create_incr_expr(lvalue_t lval, binary_oper_type_t type); // postfix
    unary_oper_t    *create_unary_oper(unary_oper_type_t type, expr_p operand);
    binary_oper_t   *create_binary_oper(binary_oper_type_t type, expr_p left, expr_p right);
    cond_expr_t     *create_cond_expr(expr_p cond, expr_p expr_true, expr_p expr_false);

    load_stmt_t     *create_load_stmt(lvalue_t lval, func_call_t call);
    break_stmt_t    *create_break_stmt() { return alloc_node<break_stmt_t>(); }
    continue_stmt_t *create_continue_stmt() { return alloc_node<continue_stmt_t>(); }
    return_stmt_t   *create_return_stmt(expr_p expr = nullptr);
    expr_stmt_t     *create_expr_stmt(expr_p expr);
    cond_stmt_t     *create_cond_stmt(expr_p cond, stmt_p stmt_true, stmt_p stmt_false = nullptr);
    loop_stmt_t     *create_loop_stmt(expr_p cond, stmt_p body, bool pre_check);
    for_loop_stmt_t *create_for_loop_stmt(stmt_p init, expr_p cond, stmt_p incr, stmt_p body);

    func_defn_t     *create_func_defn(name_t name, args_p args, stmt_p body);
    enum_defn_t     *create_enum_defn(name_t name, args_p vars, args_p values);
    import_decl_t   *create_import_decl(name_t name);
    stmt_decl_t     *create_stmt_decl(stmt_p stmt);
    infer_defn_t    *create_infer_defn(name_t scope, name_t name, stmt_p stmt);
    const_defn_t    *create_const_defn(name_t name, number_t value);

    argument_t      *create_argument(name_t name);
    argument_t      *create_argument(name_t name, number_t value);

    void register_func(name_t name, args_p args, stmt_p body);
    void register_import(name_t name);
    void register_stmt(stmt_p stmt);
    void register_infer(name_t scope, name_t name, stmt_p stmt);
    void register_infer(name_t scope, name_t name, expr_p expr);
    void register_const(name_t name, number_t value);
    void register_enum(name_t name, args_p vars, args_p values);
    void register_decl(decl_p stmt);

    template<typename T>
    T *chain(T *a, T *b) {
        if (!a) return b;
        if (!b) return a;
        const T *last = a;
        while (last->next)
            last = last->next;
        last->next = b;
        return a;
    }

};

NAMESPACE_UBSP_END;

#endif // _UBSP_SYNTAX_SCANNER_
