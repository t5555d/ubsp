#ifndef _UBSP_MACHINE_
#define _UBSP_MACHINE_

#include "ubsp-fwd.h"
#include <map>
#include <set>
#include <string>

NAMESPACE_UBSP_BEGIN;

class machine_t
{
public:
    number_t calc_unary_oper(unary_oper_type_t, number_t);
    number_t calc_binary_oper(binary_oper_type_t, number_t, number_t);

    number_t get_value(name_t var) const;
    void set_value(name_t var, number_t val);
};


NAMESPACE_UBSP_END;

#endif // _UBSP_MACHINE_