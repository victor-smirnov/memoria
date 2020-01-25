
// Copyright 2013 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/core/types.hpp>
#include <memoria/core/tools/static_array.hpp>


#include <tuple>
#include <ostream>

namespace memoria {

namespace internal {

template <int32_t Idx>
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

    template <int32_t Idx>
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

    template <int32_t Idx>
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

    template <int32_t Idx>
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

    template <int32_t Idx>
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

    template <int32_t Idx>
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

    template <int32_t Idx>
    void operator()()
    {
        out_<<std::get<Idx>(obj_);

        if (Idx < sizeof...(Args) - 1)
        {
            out_<<", ";
        }
    }
};




template <typename ElementType_, int32_t Indexes_>
void Clear(core::StaticVector<ElementType_, Indexes_>& v)
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

    template <int32_t Idx>
    void operator()()
    {
        Clear(std::get<Idx>(obj_));
    }
};

}

template <typename... Types>
void VectorAdd(std::tuple<Types...>& obj, const std::tuple<Types...>& arg)
{
    internal::DoRecursive<sizeof...(Types)>::process(internal::VectorAddFn<Types...>(obj, arg));
}

template <typename... Types>
std::tuple<Types...> operator+(const std::tuple<Types...>& obj, const std::tuple<Types...>& arg)
{
    internal::VectorAdd2Fn<Types...> fn(obj, arg);
    internal::DoRecursive<sizeof...(Types)>::process(fn);
    return fn.result_;
}


template <typename... Types>
void VectorSub(std::tuple<Types...>& obj, const std::tuple<Types...>& arg)
{
    internal::DoRecursive<sizeof...(Types)>::process(internal::VectorSubFn<Types...>(obj, arg));
}


template <typename... Types>
std::tuple<Types...> operator-(const std::tuple<Types...>& obj, const std::tuple<Types...>& arg)
{
    internal::VectorSub2Fn<Types...> fn(obj, arg);
    internal::DoRecursive<sizeof...(Types)>::process(fn);
    return fn.result_;
}


template <typename... Types>
std::tuple<Types...> operator-(const std::tuple<Types...>& obj)
{
    internal::VectorNegFn<Types...> fn(obj);
    internal::DoRecursive<sizeof...(Types)>::process(fn);
    return fn.result_;
}


template <typename... Types>
void Clear(std::tuple<Types...>& obj)
{
    internal::DoRecursive<sizeof...(Types)>::process(internal::ClearFn<Types...>(obj));
}

}

namespace std {

template <typename... Args>
ostream& operator<<(ostream& out, const tuple<Args...>& obj)
{
    out << "{";

    memoria::internal::DoRecursive<sizeof...(Args)>::process(memoria::internal::OstreamFn<Args...>(out, obj));

    out << "}";
    return out;
}

}
