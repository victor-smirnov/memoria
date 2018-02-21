/*
Copyright 2012-2017 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License, Version 1.0.
(http://www.boost.org/LICENSE_1_0.txt)
*/

#pragma once

#include <memoria/v1/reactor/smart_ptr/allocate_shared_array.hpp>

namespace memoria {
namespace v1 {
namespace reactor {

template<class T>
inline typename detail::sp_if_size_array<T>::type
make_shared_at(int32_t cpu)
{
    return reactor::allocate_shared_at<T>(cpu, std::allocator<typename
        detail::sp_array_scalar<T>::type>());
}

template<class T>
inline typename detail::sp_if_size_array<T>::type
make_shared_at(int32_t cpu, const typename detail::sp_array_element<T>::type& value)
{
    return reactor::allocate_shared_at<T>(cpu, std::allocator<typename
        detail::sp_array_scalar<T>::type>(), value);
}

template<class T>
inline typename detail::sp_if_array<T>::type
make_shared_at(int32_t cpu, std::size_t size)
{
    return reactor::allocate_shared_at<T>(cpu, std::allocator<typename
        detail::sp_array_scalar<T>::type>(), size);
}

template<class T>
inline typename detail::sp_if_array<T>::type
make_shared_at(int32_t cpu, std::size_t size,
    const typename detail::sp_array_element<T>::type& value)
{
    return reactor::allocate_shared_at<T>(cpu, std::allocator<typename
        detail::sp_array_scalar<T>::type>(), size, value);
}

template<class T>
inline typename detail::sp_if_size_array<T>::type
make_shared_noinit_at(int32_t cpu)
{
    return allocate_shared_noinit_at<T>(cpu, std::allocator<typename
        detail::sp_array_scalar<T>::type>());
}

template<class T>
inline typename detail::sp_if_array<T>::type
make_shared_noinit_at(int32_t cpu, std::size_t size)
{
    return allocate_shared_noinit_at<T>(cpu, std::allocator<typename
        detail::sp_array_scalar<T>::type>(), size);
}

}}}