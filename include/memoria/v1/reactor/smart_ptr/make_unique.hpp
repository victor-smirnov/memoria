/*
Copyright 2012-2015 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License, Version 1.0.
(http://www.boost.org/LICENSE_1_0.txt)
*/

#pragma once

#include <boost/config.hpp>
#include <memory>
#include <utility>

namespace memoria {
namespace v1 {
namespace reactor {
namespace detail {

template<class T>
struct up_if_object {
    typedef std::unique_ptr<T> type;
};

template<class T>
struct up_if_object<T[]> { };

template<class T, std::size_t N>
struct up_if_object<T[N]> { };

template<class T>
struct up_if_array { };

template<class T>
struct up_if_array<T[]> {
    typedef std::unique_ptr<T[]> type;
};

template<class T>
struct up_remove_reference {
    typedef T type;
};

template<class T>
struct up_remove_reference<T&> {
    typedef T type;
};

template<class T>
struct up_remove_reference<T&&> {
    typedef T type;
};

template<class T>
struct up_element { };

template<class T>
struct up_element<T[]> {
    typedef T type;
};

} /* detail */

template<class T>
inline typename detail::up_if_object<T>::type
make_unique()
{
    return std::unique_ptr<T>(new T());
}

template<class T, class... Args>
inline typename detail::up_if_object<T>::type
make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}


template<class T>
inline typename detail::up_if_object<T>::type
make_unique(typename detail::up_remove_reference<T>::type&& value)
{
    return std::unique_ptr<T>(new T(std::move(value)));
}

template<class T>
inline typename detail::up_if_object<T>::type
make_unique_noinit()
{
    return std::unique_ptr<T>(new T);
}

template<class T>
inline typename detail::up_if_array<T>::type
make_unique(std::size_t size)
{
    return std::unique_ptr<T>(new typename
        detail::up_element<T>::type[size]());
}

template<class T>
inline typename detail::up_if_array<T>::type
make_unique_noinit(std::size_t size)
{
    return std::unique_ptr<T>(new typename
        detail::up_element<T>::type[size]);
}

}}}
