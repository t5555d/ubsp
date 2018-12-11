#ifndef _UBSP_VARIABLE_
#define _UBSP_VARIABLE_

#include "ubsp-fwd.h"
#include <vector>

NAMESPACE_UBSP_BEGIN;

constexpr size_t MIN_ARRAY_SIZE = 8;

class array_t
{
public:
    array_t(number_t v) : ndims(0), value{v} {}

    array_t(int n, number_t i[], number_t v):
        ndims(n)
    {
        value.dim = nullptr;
        put(ndims, i, v);
    }

    array_t(array_t&& that) :
        ndims(that.ndims), value(that.value)
    {
        that.value.dim = nullptr;
    }

    void operator=(array_t&& that) {
        if (this == &that) return;
        cleanup(0, value.dim);
        value = that.value;
        that.value.dim = nullptr;
    }

    ~array_t() {
        cleanup(0, value.dim);
    }

    number_t get(int ndims, number_t index[]);
    void put(int ndims, number_t index[], number_t value);
    int num_dimensions() const { return ndims; }

private:

    struct dimension_t;

    union variant_t
    {
        number_t number;
        dimension_t *dim;
    };

    struct dimension_t
    {
        size_t size;
        size_t reserved;
        variant_t values[MIN_ARRAY_SIZE];
    };

    dimension_t *assure_size(dimension_t *dim, size_t size);
    void cleanup(int depth, dimension_t *dim);

    variant_t value;
    const int ndims; // number of dimensions
};


NAMESPACE_UBSP_END;

#endif // _UBSP_VARIABLE_