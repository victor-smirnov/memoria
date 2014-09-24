
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_VECTORTUPLE_H
#define _MEMORIA_CORE_TOOLS_VECTORTUPLE_H


#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/static_array.hpp>


#include <tuple>
#include <ostream>

namespace memoria {

namespace internal {

template <Int Idx>
struct DoRecursive {
    template <typename Fn>
    static void process(Fn&& fn)
    {
        DoRecursive<Idx - 1>::process(std::forward<Fn>(fn));

        fn.template operator()<Idx - 1>();
    }
};

template <>
struct DoRecursive<0> {
    template <typename Fn>
    static void process(Fn&& fn) {}
};

template <typename... Args>
struct VectorAddFn {
    typedef std::tuple<Args...> Tuple;

    Tuple& obj_;
    const Tuple& arg_;

    VectorAddFn(Tuple& obj, const Tuple& arg): obj_(obj), arg_(arg) {}

    template <Int Idx>
    void operator()()
    {
        std::get<Idx>(obj_) += std::get<Idx>(arg_);
    }
};

template <typename... Args>
struct VectorAdd2Fn {
    typedef std::tuple<Args...> Tuple;

    const Tuple& obj_;
    const Tuple& arg_;

    Tuple result_;

    VectorAdd2Fn(const Tuple& obj, const Tuple& arg): obj_(obj), arg_(arg) {}

    template <Int Idx>
    void operator()()
    {
        std::get<Idx>(result_) = std::get<Idx>(obj_) + std::get<Idx>(arg_);
    }
};


template <typename... Args>
struct VectorSubFn {
    typedef std::tuple<Args...> Tuple;

    Tuple& obj_;
    const Tuple& arg_;

    VectorSubFn(Tuple& obj, const Tuple& arg): obj_(obj), arg_(arg) {}

    template <Int Idx>
    void operator()()
    {
        std::get<Idx>(obj_) -= std::get<Idx>(arg_);
    }
};


template <typename... Args>
struct VectorSub2Fn {
    typedef std::tuple<Args...> Tuple;

    const Tuple& obj_;
    const Tuple& arg_;

    Tuple result_;

    VectorSub2Fn(const Tuple& obj, const Tuple& arg): obj_(obj), arg_(arg) {}

    template <Int Idx>
    void operator()()
    {
        std::get<Idx>(result_) = std::get<Idx>(obj_) - std::get<Idx>(arg_);
    }
};

template <typename... Args>
struct VectorNegFn {
    typedef std::tuple<Args...> Tuple;

    const Tuple& obj_;
    Tuple result_;

    VectorNegFn(const Tuple& obj): obj_(obj) {}

    template <Int Idx>
    void operator()()
    {
        std::get<Idx>(result_) = -std::get<Idx>(obj_);
    }
};

template <typename... Args>
struct OstreamFn {
    typedef std::tuple<Args...> Tuple;
    std::ostream& out_;
    const Tuple& obj_;

    OstreamFn(std::ostream& out, const Tuple& obj): out_(out), obj_(obj) {}

    template <Int Idx>
    void operator()()
    {
        out_<<std::get<Idx>(obj_);

        if (Idx < sizeof...(Args) - 1)
        {
            out_<<", ";
        }
    }
};




template <typename ElementType_, Int Indexes_>
void Clear(::memoria::core::StaticVector<ElementType_, Indexes_>& v)
{
	v.clear();
}

template <typename T>
void Clear(T& t) {
	t = 0;
}



template <typename... Args>
struct ClearFn {
    typedef std::tuple<Args...> Tuple;
    Tuple& obj_;

    ClearFn(Tuple& obj): obj_(obj) {}

    template <Int Idx>
    void operator()()
    {
        Clear(get<Idx>(obj_));
    }
};

}

template <typename... Types>
void VectorAdd(std::tuple<Types...>& obj, const std::tuple<Types...>& arg)
{
    memoria::internal::DoRecursive<sizeof...(Types)>::process(memoria::internal::VectorAddFn<Types...>(obj, arg));
}

template <typename... Types>
std::tuple<Types...> operator+(const std::tuple<Types...>& obj, const std::tuple<Types...>& arg)
{
    memoria::internal::VectorAdd2Fn<Types...> fn(obj, arg);
    memoria::internal::DoRecursive<sizeof...(Types)>::process(fn);
    return fn.result_;
}


template <typename... Types>
void VectorSub(std::tuple<Types...>& obj, const std::tuple<Types...>& arg)
{
    memoria::internal::DoRecursive<sizeof...(Types)>::process(memoria::internal::VectorSubFn<Types...>(obj, arg));
}


template <typename... Types>
std::tuple<Types...> operator-(const std::tuple<Types...>& obj, const std::tuple<Types...>& arg)
{
    memoria::internal::VectorSub2Fn<Types...> fn(obj, arg);
    memoria::internal::DoRecursive<sizeof...(Types)>::process(fn);
    return fn.result_;
}


template <typename... Types>
std::tuple<Types...> operator-(const std::tuple<Types...>& obj)
{
    memoria::internal::VectorNegFn<Types...> fn(obj);
    memoria::internal::DoRecursive<sizeof...(Types)>::process(fn);
    return fn.result_;
}


template <typename... Types>
void Clear(std::tuple<Types...>& obj)
{
    memoria::internal::DoRecursive<sizeof...(Types)>::process(memoria::internal::ClearFn<Types...>(obj));
}





}

namespace std {

template <typename... Args>
ostream& operator<<(ostream& out, const ::memoria::tuple<Args...>& obj)
{
    out<<"{";

    memoria::internal::DoRecursive<sizeof...(Args)>::process(memoria::internal::OstreamFn<Args...>(out, obj));

    out<<"}";
    return out;
}
}


#endif
