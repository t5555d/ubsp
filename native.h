#ifndef _UBSP_NATIVE_
#define _UBSP_NATIVE_

#include "ubsp-fwd.h"
#include "utils.h"
#include <utility>
#include <tuple>

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

// function/method traits
template<typename T> struct function_traits {};

template<typename ReturnType, typename... Args>
struct function_traits<ReturnType(*)(Args...)>
{
    typedef ReturnType result_type;
    static constexpr int argc = sizeof...(Args);
    typedef std::tuple<Args...> argv;
};

template<typename ReturnType, typename ClassType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...)>
{
    typedef ReturnType result_type;
    typedef ClassType class_type;
    typedef std::tuple<Args...> argv;
    static constexpr int argc = sizeof...(Args);
};

template<typename ReturnType, typename ClassType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...) const>
{
    typedef ReturnType result_type;
    typedef const ClassType class_type;
    typedef std::tuple<Args...> argv;
    static constexpr int argc = sizeof...(Args);
};

// native functions/methods export utilities

template <typename Context, typename Method, size_t... Idx>
typename std::enable_if<!std::is_void<typename function_traits<Method>::result_type>::value, number_t>::type
native_func_call(Context *context, Method method, number_t argv[MAX_ARGS], std::index_sequence<Idx...>)
{
    constexpr function_traits<Method>::argv argv_types;
    auto result = (context->*method)(static_cast<decltype(std::get<Idx>(argv_types))>(argv[Idx])...);
    return static_cast<number_t>(result);
}

template <typename Context, typename Method, size_t... Idx>
typename std::enable_if<std::is_void<typename function_traits<Method>::result_type>::value, number_t>::type
native_func_call(Context *context, Method method, number_t argv[MAX_ARGS], std::index_sequence<Idx...>)
{
    constexpr function_traits<Method>::argv argv_types;
    (context->*method)(static_cast<decltype(std::get<Idx>(argv_types))>(argv[Idx])...);
    return 0;
}

template<typename Context, typename Method, Method method>
number_t native_func_impl(Context *context, int argc, number_t argv[MAX_ARGS])
{
    constexpr int EXP_ARGC = function_traits<Method>::argc;
    if (argc != EXP_ARGC) {
        throw wrong_argc_error{ nullptr, EXP_ARGC, argc };
    }

    return native_func_call(context, method, argv, std::make_index_sequence<EXP_ARGC>());
}

#define EXPORT_FUNCTION(context, function) { #function, context::function }
#define EXPORT_METHOD(context, method) { #method, native_func_impl<context, decltype(&context::method), &context::method> }

export_record_t<void> native_math[];

NAMESPACE_UBSP_END;

#endif // !_UBSP_NATIVE_
