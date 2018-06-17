#ifndef _UBSP_OPERATORS_
#define _UBSP_OPERATORS_

#include "ubsp-fwd.h"

NAMESPACE_UBSP_BEGIN;

typedef number_t (*calc_fn)(const number_t *stack);

struct operator_info_t {
    const char *text;
    expr_priority_t priority;
    calc_fn calc;
};

extern const operator_info_t binary_oper_info[];
extern const operator_info_t unary_oper_info[];

inline const operator_info_t& get_operator_info(binary_oper_type_t type) {
    return binary_oper_info[int(type)];
}

inline const operator_info_t& get_operator_info(unary_oper_type_t type) {
    return unary_oper_info[int(type)];
}

NAMESPACE_UBSP_END;

#endif // !_UBSP_OPERATORS_
