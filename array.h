#ifndef _UBSP_VARIABLE_
#define _UBSP_VARIABLE_

#include "ubsp-fwd.h"
#include <vector>

NAMESPACE_UBSP_BEGIN;

struct wrong_ndims_error {
    int req_ndims;
    int max_ndims;
};

struct wrong_index_error {
    number_t index;
    size_t size;
    int dim;
};

struct array_not_init_error {
    int dim;
};

constexpr size_t MIN_ARRAY_SIZE = 14;

class array_t
{
public:
    array_t(number_t v) : ndims(0), value{v} {}

    array_t(int n, number_t index[], number_t value):
        ndims(n)
    {
        put(ndims, index, value);
    }

    ~array_t() {
        if (0 < ndims)
            cleanup(0, value.dim);
    }

    number_t get(int ndims, number_t index[]);
    void put(int ndims, number_t index[], number_t value);

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