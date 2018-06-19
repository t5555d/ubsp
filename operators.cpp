#include "operators.h"

NAMESPACE_UBSP_BEGIN;

namespace {

#define BINARY_OPER_IMPL(name, oper) number_t name(number_t a, number_t b) { return a oper b; }
#define UNARY_OPER_IMPL(name, oper) number_t name(number_t a, number_t b) { return oper a; }

    BINARY_OPER_IMPL(add, +);
    BINARY_OPER_IMPL(sub, -);
    BINARY_OPER_IMPL(mul, *);
    BINARY_OPER_IMPL(div, /);
    BINARY_OPER_IMPL(mod, %);

    BINARY_OPER_IMPL(shl, <<);
    BINARY_OPER_IMPL(shr, >>);

    BINARY_OPER_IMPL(band, &);
    BINARY_OPER_IMPL(bor, |);
    BINARY_OPER_IMPL(xor, ^);

    BINARY_OPER_IMPL(and, &&);
    BINARY_OPER_IMPL(or, ||);

    BINARY_OPER_IMPL(eq, ==);
    BINARY_OPER_IMPL(ne, !=);
    BINARY_OPER_IMPL(lt, <);
    BINARY_OPER_IMPL(le, <=);
    BINARY_OPER_IMPL(gt, >);
    BINARY_OPER_IMPL(ge, >=);

    UNARY_OPER_IMPL(neg, -);
    UNARY_OPER_IMPL(not, !);
    UNARY_OPER_IMPL(bnot, ~);

}

const operator_info_t binary_oper_info[] = {
    { "",   NOP_PRIORITY, nullptr },

    { "+",  ADD_PRIORITY, add },
    { "-",  ADD_PRIORITY, sub },
    { "*",  MUL_PRIORITY, mul },
    { "/",  MUL_PRIORITY, div },
    { "%",  MUL_PRIORITY, mod },

    { "<<", SHL_PRIORITY, shl },
    { ">>", SHL_PRIORITY, shr },

    { "&",  BAND_PRIORITY, band },
    { "|",  BOR_PRIORITY, bor },
    { "^",  XOR_PRIORITY, xor },

    { "&&", AND_PRIORITY, and },
    { "||", OR_PRIORITY, or },

    { "==", EQ_PRIORITY, eq },
    { "!=", EQ_PRIORITY, ne },
    { "<",  CMP_PRIORITY, lt },
    { "<=", CMP_PRIORITY, le },
    { ">",  CMP_PRIORITY, gt },
    { ">=", CMP_PRIORITY, ge },
};

const operator_info_t unary_oper_info[] = {
    { "-", PREFIX_PRIORITY, neg },
    { "!", PREFIX_PRIORITY, not },
    { "~", PREFIX_PRIORITY, bnot },
};

static_assert(sizeof(binary_oper_info) / sizeof(operator_info_t) == NUM_BINARY_OPER_TYPES, "Wrong number of records");
static_assert(sizeof(unary_oper_info) / sizeof(operator_info_t) == NUM_UNARY_OPER_TYPES, "Wrong number of records");

NAMESPACE_UBSP_END;