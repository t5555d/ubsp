#include "operators.h"

NAMESPACE_UBSP_BEGIN;

const operator_info_t binary_oper_info[] = {
    { "",   NOP_PRIORITY },

    { "+",  ADD_PRIORITY },
    { "-",  ADD_PRIORITY },
    { "*",  MUL_PRIORITY },
    { "/",  MUL_PRIORITY },
    { "%",  MUL_PRIORITY },

    { "<<", SHL_PRIORITY },
    { ">>", SHL_PRIORITY },

    { "&",  BAND_PRIORITY },
    { "|",  BOR_PRIORITY },
    { "^",  XOR_PRIORITY },

    { "&&", AND_PRIORITY },
    { "||", OR_PRIORITY },

    { "==", EQ_PRIORITY },
    { "!=", EQ_PRIORITY },
    { "<",  CMP_PRIORITY },
    { "<=", CMP_PRIORITY },
    { ">",  CMP_PRIORITY },
    { ">=", CMP_PRIORITY },
};

const operator_info_t unary_oper_info[] = {
    { "-", PREFIX_PRIORITY },
    { "!", PREFIX_PRIORITY },
    { "~", PREFIX_PRIORITY },
};

static_assert(sizeof(binary_oper_info) / sizeof(operator_info_t) == NUM_BINARY_OPER_TYPES, "Wrong number of records");
static_assert(sizeof(unary_oper_info) / sizeof(operator_info_t) == NUM_UNARY_OPER_TYPES, "Wrong number of records");

NAMESPACE_UBSP_END;