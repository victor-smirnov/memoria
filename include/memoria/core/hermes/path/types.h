/****************************************************************************
**
** Author: Róbert Márki <gsmiko@gmail.com>
** Copyright (c) 2016 Róbert Márki
** Copyright (c) 2022 Victor Smirnov
**
** This file is originally based on the jmespath.cpp project
** (https://github.com/robertmrk/jmespath.cpp, commitid: 9c9702a)
** and is distributed under the MIT License (MIT).
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to
** deal in the Software without restriction, including without limitation the
** rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
** sell copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
** FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
**
****************************************************************************/
#ifndef TYPES_H
#define TYPES_H
#include <string>
#include <limits>
#include <boost/regex/pending/unicode_iterator.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <nlohmann/json.hpp>

#include <memoria/core/hermes/hermes.hpp>

namespace memoria {
namespace hermes {
namespace path {
/**
 * @brief 8 bit character type
 */
using Char              = char;
/**
 * @brief UTF-8 encoded string type
 */
using String            = std::basic_string<Char>;
/**
 * @brief 32 bit character type
 */
using UnicodeChar       = char32_t;
/**
 * @brief UTF-32 encoded string type
 */
using UnicodeString     = std::basic_string<UnicodeChar>;
/**
 * @brief UTF-32 string iterator adaptor
 */
using UnicodeIteratorAdaptor
    = boost::u8_to_u32_iterator<String::const_iterator>;

using U8UnicodeIteratorAdaptor
    = boost::u8_to_u32_iterator<U8StringView::const_iterator>;

/**
 * @brief UTF-8 string iterator adaptor
 */
using StringIteratorAdaptor
    = boost::u32_to_u8_iterator<UnicodeString::const_iterator>;

/**
 * @brief Signed integer type that can hold all values in the range of
 * numeric_limits<size_t>::max() * -1 ... numeric_limits<size_t>::max()
 */
using Index = boost::multiprecision::number<
    boost::multiprecision::cpp_int_backend<
        std::numeric_limits<size_t>::digits,
        std::numeric_limits<size_t>::digits,
        boost::multiprecision::signed_magnitude,
        boost::multiprecision::checked,
        void> >;
}
}}
#endif // TYPES_H
