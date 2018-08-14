#include "native.h"
#include <math.h>

NAMESPACE_UBSP_BEGIN;

number_t exp_log2(void *context, int argc, number_t argv[MAX_ARGS]) {
    if (argc != 1) throw wrong_argc_error{ "log2", 1, argc };
    double result = log2(static_cast<double>(argv[0]));
    return static_cast<number_t>(ceil(result));
}

number_t exp_ceil(void *context, int argc, number_t argv[MAX_ARGS]) {
    if (argc != 1) throw wrong_argc_error{ "ceil", 1, argc };
    return argv[0];
}

export_record_t<void> native_math[] = {
    { "log2", exp_log2 },
    { "ceil", exp_ceil },
    { nullptr }
};

NAMESPACE_UBSP_END;