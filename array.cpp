#include "array.h"
#include "error.h"
#include <algorithm>

NAMESPACE_UBSP_BEGIN;

number_t array_t::get(int n, number_t index[])
{
    variant_t *v = &value;
    for (int i = 0; i < ndims; i++) {
        if (v->dim == nullptr)
            throw array_not_init_error{ i };
        if (index[i] < 0 || size_t(index[i]) >= v->dim->size)
            throw wrong_index_error{ index[i], v->dim->size, i };
        v = &v->dim->values[index[i]];
    }

    return v->number;
}

void array_t::put(int n, number_t index[], number_t num)
{
    variant_t *v = &value;
    for (int i = 0; i < ndims; i++) {
        v->dim = assure_size(v->dim, index[i] + 1);
        v = &v->dim->values[index[i]];
    }
    v->number = num;
}

array_t::dimension_t *array_t::assure_size(array_t::dimension_t *dim, size_t size)
{
    if (dim != nullptr && dim->reserved >= size) {
        dim->size = size;
        return dim;
    }

    size_t reserved = 1;
    while (reserved < size) reserved *= 2;
    reserved = std::max(reserved, MIN_ARRAY_SIZE);
    size_t required_size = sizeof(dimension_t) + (reserved - MIN_ARRAY_SIZE) * sizeof(variant_t);
    if (dim) {
        dim = (dimension_t *)realloc(dim, required_size);
        memset(dim->values + dim->reserved, 0, (reserved - dim->reserved) * sizeof(variant_t));
    }
    else {
        dim = (dimension_t *)malloc(required_size);
        memset(dim->values, 0, reserved * sizeof(variant_t));
    }
    dim->reserved = reserved;
    dim->size = size;
    return dim;
}

void array_t::cleanup(int depth, array_t::dimension_t *dim)
{
    if (depth >= ndims || dim == nullptr)
        return;

    for (int32_t i = 0; i < dim->reserved; i++)
        cleanup(depth + 1, dim->values[i].dim);
    free(dim);
}

NAMESPACE_UBSP_END;