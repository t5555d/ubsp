#ifndef _UBSP_NATIVE_
#define _UBSP_NATIVE_

#include "ubsp-fwd.h"
#include "utils.h"

NAMESPACE_UBSP_BEGIN;

constexpr int MAX_ARGS = 30;

template<typename Context>
using native_func_t = number_t(*)(Context *context, int argc, number_t argv[MAX_ARGS]);

struct wrong_argc_error {
    const char *func;
    int required_argc;
    int provided_argc;
};

template<typename Context>
struct export_record_t
{
    const char *name;
    native_func_t<Context> func;
};

NAMESPACE_UBSP_END;

#endif // !_UBSP_NATIVE_
