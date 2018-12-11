#pragma once

#include <exception>
#include <ostream>
#include "ubsp-fwd.h"

NAMESPACE_UBSP_BEGIN;

struct error_t : public std::exception
{
    virtual void explain(std::ostream& out) const = 0;
};

struct undef_module_error : error_t
{
    name_t name;
    
    undef_module_error(name_t n) : name(n) {}
    const char *what() const override { return "Undefined module"; }
    void explain(std::ostream& out) const override {
        out << "Undefined module: " << name;
    }
};

struct undef_var_error : error_t
{
    name_t name;
    
    undef_var_error(name_t n) : name(n) {}
    const char *what() const override { return "Undefined variable"; }
    void explain(std::ostream& out) const override {
        out << "Undefined variable: " << name;
    }
};

struct undef_func_error : error_t
{
    name_t name;
    
    undef_func_error(name_t n) : name(n) {}
    const char *what() const override { return "Undefined function"; }
    void explain(std::ostream& out) const override {
        out << "Undefined function: " << name;
    }
};

struct infer_var_error : error_t
{
    name_t name;
    
    infer_var_error(name_t n) : name(n) {}
    const char *what() const override { return "Failed to infer variable"; }
    void explain(std::ostream& out) const override {
        out << "Failed to infer variable: " << name;
    }
};

struct const_ndims_error : error_t
{
    name_t name;
    int ndims;
    
    const_ndims_error(name_t n, int d) : 
        name(n), ndims(d) {}
    const char *what() const override { return "Wrong dimensions for constant"; }
    void explain(std::ostream& out) const override {
        out << "Wrong dimensions for a constant: " << name;
    }
};

struct const_value_error : error_t
{
    name_t name;
    number_t value;
    number_t const_value;
    
    const_value_error(name_t n, number_t v, number_t c) :
        name(n), value(v), const_value(c) {}
    const char *what() const override { return "Wrong dimensions for constant"; }
    void explain(std::ostream& out) const override {
        out << "Wrong value for a constant " << name << ": expected: " << const_value << ", actual: " << value;
    }
};

struct dup_func_defn_error : error_t
{
    name_t name;
    
    dup_func_defn_error(name_t n) : name(n) {}
    const char *what() const override { return "Duplicate definition of function"; }
    void explain(std::ostream& out) const override {
        out << "Duplicate definition of function: " << name;
    }
};

struct dup_var_infer_error : error_t
{
    name_t name;
    
    dup_var_infer_error(name_t n) : name(n) {}
    const char *what() const override { return "Duplicate infer"; }
    void explain(std::ostream& out) const override {
        out << "Duplicate infer of variable: " << name;
    }
};

struct dup_var_write_error : error_t
{
    name_t name;
    name_t func1;
    name_t func2;
    
    dup_var_write_error(name_t n, name_t f1, name_t f2) :
        name(n), func1(f1), func2(f2) {}
    const char *what() const override { return "Multiple functions write global variable"; }
    void explain(std::ostream& out) const override {
        out << "Multiple functions write global variable " << name << ": " << func1 << " and " << func2;
    }
};

struct wrong_ndims_error : error_t
{
    name_t name;
    int actual_ndims;
    int expect_ndims;

    wrong_ndims_error(name_t n, int act, int exp) :
        name(n), actual_ndims(act), expect_ndims(exp) {}
    const char *what() const override { return "Wrong variable dimensions"; }
    void explain(std::ostream& out) const override {
        out << "Wrong dimensions of variable " << name << ": actual " << actual_ndims << ", expected " << expect_ndims;
    }
};

struct wrong_index_error : error_t
{
    name_t name = nullptr;
    number_t index;
    size_t size;
    int dim;

    wrong_index_error(number_t i, size_t s, int d) :
        index(i), size(s), dim(d) {}
    const char *what() const override { return "Index out of bounds"; }
    void explain(std::ostream& out) const override {
        out << "Index out of bounds: variable " << name << ", dimension " << dim << ", index " << index << " >= " << size;
    }
};

struct array_not_init_error : error_t
{
    name_t name = nullptr;
    int dim;

    array_not_init_error(int d) : dim(d) {}
    const char *what() const override { return "Array dimension not initialized"; }
    void explain(std::ostream& out) const override {
        out << "Index dimension not initialized: variable " << name << ", dimension " << dim;
    }
};

NAMESPACE_UBSP_END;