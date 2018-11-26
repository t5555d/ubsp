#ifndef _UBSP_MACHINE_
#define _UBSP_MACHINE_

#include "ubsp-fwd.h"
#include "array.h"
#include "native.h"
#include <map>
#include <list>
#include <string>

NAMESPACE_UBSP_BEGIN;

class machine_t : private syntax_processor_i
{
public:

    machine_t(syntax_t& syntax);
    void execute();

    number_t get(const lvalue_t& lval);
    void put(const lvalue_t& lval, number_t n);

    template<typename T>
    void export_native_object(const char *name, T *context, const export_record_t<T> *exports) {
        native_objects.emplace(name, native_object_t{ context, (const export_record_t<void> *) exports });
    }

    template<typename T>
    void export_native_object(const char *name, T& context, const export_record_t<T> *exports) {
        native_objects.emplace(name, native_object_t{ &context, (const export_record_t<void> *) exports });
    }

    void export_native_object(const char *name, const export_record_t<void> *exports) {
        native_objects.emplace(name, native_object_t{ nullptr, exports });
    }

private:
    typedef std::map<name_t, array_t> var_scope_t;

    int eval_args(number_t argv[MAX_ARGS], expr_p expr);
    int eval_args(number_t argv[MAX_ARGS], const func_call_t& call);

    syntax_t& syntax;

    struct native_object_t
    {
        void *context;
        const export_record_t<void> *exports;
    };

    struct native_method_t
    {
        void *context;
        native_func_t<void> func;
    };

    std::map<std::string, native_object_t> native_objects;
    std::map<std::string, native_method_t> native_methods;

    typedef std::map<name_t, stmt_p> infer_map_t;

    std::map<name_t, const func_defn_t *> funcs;
    infer_map_t global_infers;
    std::map<name_t, infer_map_t> scoped_infers;
    infer_map_t *scope_infers;

    std::list<stmt_p> stmts;
    var_scope_t global_scope, memory_scope, *scope;
    number_t value;

    array_t& find(name_t name);

    number_t call(const func_defn_t& func, int argc, number_t argv[MAX_ARGS]);
    number_t call(const func_call_t& call);
    number_t eval(expr_p expr, number_t default_value = 0);
    void exec(stmt_p stmt);

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
    virtual void process(const import_decl_t&) override;
    virtual void process(const infer_defn_t&) override;
};

struct undef_var_error {
    name_t name;
};

struct infer_var_error {
    name_t name;
};

struct undef_func_error {
    name_t name;
};

NAMESPACE_UBSP_END;

#endif // _UBSP_MACHINE_