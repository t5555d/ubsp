#ifndef _UBSP_UTILS_
#define _UBSP_UTILS_

#include "ubsp-fwd.h"

NAMESPACE_UBSP_BEGIN;

template<typename T>
class value_restore
{
    T prev_value;
    T& value;
public:
    value_restore(T& v) : value(v), prev_value(v) {}
    ~value_restore() { value = prev_value; }
};

template<typename T>
value_restore<T> remember(T& var)
{
    return value_restore<T>(var);
}


NAMESPACE_UBSP_END;

#endif // _UBSP_UTILS_