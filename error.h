#pragma once

#include <exception>
#include <ostream>
#include "ubsp-fwd.h"

NAMESPACE_UBSP_BEGIN;

struct error_t : public std::exception
{
    virtual void explain(std::ostream& out) const = 0;
};

enum entity_t
{
    VARIABLE,
    FUNCTION,
    MODULE,
    INFER_DEFN,
    CONST_DEFN,
    ENUM_DEFN,
};

template<entity_t entity>
struct entity_traits {};

template<>
struct entity_traits<VARIABLE>
{
    static constexpr const char *NAME = "variable";
};

template<>
struct entity_traits<FUNCTION>
{
    static constexpr const char *NAME = "function";
};

template<>
struct entity_traits<MODULE>
{
    static constexpr const char *NAME = "module";
};

template<>
struct entity_traits<INFER_DEFN>
{
    static constexpr const char *NAME = "infer";
};

template<>
struct entity_traits<CONST_DEFN>
{
    static constexpr const char *NAME = "const";
};

template<>
struct entity_traits<ENUM_DEFN>
{
    static constexpr const char *NAME = "enum";
};

template<entity_t ENTITY>
struct undef_error : error_t
{
    name_t name;
    
    undef_error(name_t n) : name(n) {}
    void explain(std::ostream& out) const override {
        out << "Undefined " << entity_traits<ENTITY>::NAME << ": " << name;
    }
};

template<entity_t ENTITY>
struct duplicate_error : error_t
{
    name_t name;

    duplicate_error(name_t n) : name(n) {}
    void explain(std::ostream& out) const override {
        out << "Duplicate " << entity_traits<ENTITY>::NAME << ": " << name;
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

struct dup_var_decl_error : error_t
{
    name_t name;
    name_t type1;
    name_t type2;

    dup_var_decl_error(name_t n, name_t t1, name_t t2) :
        name(n), type1(t1), type2(t2) {}
    const char *what() const override { return "Duplicate variable declaration"; }
    void explain(std::ostream& out) const override {
        name_t t1 = type1 ? type1 : "(anonymous)";
        name_t t2 = type2 ? type2 : "(anonymous)";
        out << "Variable " << name << " is declared with different types: " << t1 << " vs " << t2;
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