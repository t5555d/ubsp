#ifndef _UBSP_UTILS_
#define _UBSP_UTILS_

#include "ubsp-fwd.h"

NAMESPACE_UBSP_BEGIN;

template<typename Func>
class destruct_caller
{
    Func func;
public:
    destruct_caller(Func f) : func(f) {}
    ~destruct_caller() { func(); }
};


template<typename Func>
destruct_caller<Func> on_exit_scope(Func f)
{
    return destruct_caller<Func>(f);
}

NAMESPACE_UBSP_END;

#endif // _UBSP_UTILS_