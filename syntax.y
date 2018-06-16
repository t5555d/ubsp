%locations
%define api.pure
%output "syntax.tab.cpp"

%code top {
    #define YYERROR_VERBOSE 1
    #define YYDEBUG 1
}

%code requires {
    #include "syntax.h"
}

%union {
    ubsp::number_t        number;
    ubsp::name_t           ident;
    ubsp::func_call_t       call;
    ubsp::lvalue_t          lval;
    ubsp::binary_oper_type_t  bt;
    ubsp::unary_oper_type_t   ut;
    ubsp::scope_exit_type_t   et;
    ubsp::syntax_node_i    *node;
    ubsp::statement_i      *stmt;
    ubsp::expression_i     *expr;
}

%{
    #include "syntax-loader.h"
    using namespace ubsp;
%}

%lex-param {ubsp::syntax_loader_t *runtime}
%parse-param {ubsp::syntax_loader_t *runtime}

%type <stmt> stmt stmt0N
%type <stmt> init init1N init0N
%type <expr> expr args1N args0N 
%type <expr> chng index0N
%type <call> call
%type <lval> lval

%token <number> NUMBER
%token <ident> IDENT 
%token IF ELSE
%token DO WHILE FOR
%token <et> RETURN BREAK CONTINUE

%nonassoc RETURN
%nonassoc IDENT
%nonassoc ')'
%nonassoc ELSE
%right <bt> '=' CHANGE
%right '?' ':'
%left <bt> OR
%left <bt> AND
%left <bt> '|'
%left <bt> '^'
%left <bt> '&'
%left <bt> EQ NE
%left <bt> '<' LE '>' GE
%left <bt> SHL SHR
%left <bt> '+' '-'
%left <bt> '*' '/' '%'
%right <ut> NEG '!' '~'
%right <bt> INC DEC
%left POSTFIX

%%

input: defn 
    | defn input

defn: stmt                          { runtime->register_stmt($1); }
    | IDENT '(' ')' '{' stmt0N '}'  { runtime->register_func($1, $5); }

/* definitions */

lval: IDENT index0N             { $$ = { $1, $2 }; }
call: IDENT '(' args0N ')'      { $$ = { $1, $3 }; }
init: chng                      { $$ = runtime->create_expr_stmt($1); }

chng: call                      { $$ = runtime->create_call_expr($1); }
    | lval '=' expr             { $$ = runtime->create_chng_expr($1, $3, $2); }
    | lval CHANGE expr          { $$ = runtime->create_chng_expr($1, $3, $2); }
    | INC lval                  { $$ = runtime->create_incr_expr($1, $2); }
    | DEC lval                  { $$ = runtime->create_incr_expr($1, $2); }
    | lval INC %prec POSTFIX    { $$ = runtime->create_incr_expr($1, $2); }
    | lval DEC %prec POSTFIX    { $$ = runtime->create_incr_expr($1, $2); }

stmt: '{' stmt0N '}'            { $$ = $2; }
	| init						{ $$ = $1; }
    | lval call                 { $$ = runtime->create_load_stmt($1, $2); }
    | RETURN                    { $$ = runtime->create_exit_stmt($1); }
    | RETURN expr               { $$ = runtime->create_exit_stmt($1, $2); }
    | BREAK                     { $$ = runtime->create_exit_stmt($1); }
    | CONTINUE                  { $$ = runtime->create_exit_stmt($1); }
    | IF '(' expr ')' stmt      { $$ = runtime->create_cond_stmt($3, $5); }
    | IF '(' expr ')' stmt ELSE stmt { $$ = runtime->create_cond_stmt($3, $5, $7); }
    | WHILE '(' expr ')' stmt   { $$ = runtime->create_loop_stmt($3, $5, true); }
    | DO stmt WHILE '(' expr ')' { $$ = runtime->create_loop_stmt($5, $2, false); }
    | FOR '(' init0N ';' expr ';' init0N ')' stmt {
        auto loop = runtime->create_for_loop_stmt($5, $9, $7);
        $$ = runtime->chain<statement_i>($3, loop); }
    | FOR '(' init0N ';' ';' init0N ')' stmt {
        auto loop = runtime->create_for_loop_stmt(nullptr, $8, $6);
        $$ = runtime->chain<statement_i>($3, loop); }


expr: '(' expr ')'      { $$ = $2; }
    | chng              { $$ = $1; }
    | NUMBER            { $$ = runtime->create_const_expr($1); }
    | lval              { $$ = runtime->create_lval_expr($1); }
    | expr '?' expr ':' expr { $$ = runtime->create_cond_expr($1, $3, $5); }
    | expr OR expr      { $$ = runtime->create_binary_oper($2, $1, $3); }
    | expr AND expr     { $$ = runtime->create_binary_oper($2, $1, $3); }
    | expr '|' expr     { $$ = runtime->create_binary_oper($2, $1, $3); }
    | expr '^' expr     { $$ = runtime->create_binary_oper($2, $1, $3); }
    | expr '&' expr     { $$ = runtime->create_binary_oper($2, $1, $3); }
    | expr EQ expr      { $$ = runtime->create_binary_oper($2, $1, $3); }
    | expr NE expr      { $$ = runtime->create_binary_oper($2, $1, $3); }
    | expr '<' expr     { $$ = runtime->create_binary_oper($2, $1, $3); }
    | expr LE expr      { $$ = runtime->create_binary_oper($2, $1, $3); }
    | expr '>' expr     { $$ = runtime->create_binary_oper($2, $1, $3); }
    | expr GE expr      { $$ = runtime->create_binary_oper($2, $1, $3); }
    | expr SHL expr     { $$ = runtime->create_binary_oper($2, $1, $3); }
    | expr SHR expr     { $$ = runtime->create_binary_oper($2, $1, $3); }
    | expr '+' expr     { $$ = runtime->create_binary_oper($2, $1, $3); }
    | expr '-' expr     { $$ = runtime->create_binary_oper($2, $1, $3); }
    | expr '*' expr     { $$ = runtime->create_binary_oper($2, $1, $3); }
    | expr '/' expr     { $$ = runtime->create_binary_oper($2, $1, $3); }
    | expr '%' expr     { $$ = runtime->create_binary_oper($2, $1, $3); }
    | '-' expr %prec NEG{ $$ = runtime->create_unary_oper(unary_oper_type_t::NEG, $2); }
    | '!' expr          { $$ = runtime->create_unary_oper($1, $2); }
    | '~' expr          { $$ = runtime->create_unary_oper($1, $2); }

/* various types of lists */

args1N: expr | expr ',' args1N  { $$ = runtime->chain($1, $3); }
args0N: args1N | /*empty*/      { $$ = nullptr; }

index0N: /*empty*/              { $$ = nullptr; }
     | '[' expr ']' index0N     { $$ = runtime->chain($2, $4); }

init1N: init | init ',' init1N  { $$ = runtime->chain($1, $3); }
init0N: init1N | /*empty*/      { $$ = nullptr; }

stmt0N: /*empty*/               { $$ = nullptr; }
    | stmt stmt0N               { $$ = runtime->chain($1, $2); }
