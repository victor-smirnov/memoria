/****************************************************************************
**
** Author: Róbert Márki <gsmiko@gmail.com>
** Copyright (c) 2016 Róbert Márki
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
#ifndef ENCODESURROGATEPAIRACTION_H
#define ENCODESURROGATEPAIRACTION_H
#include "memoria/core/hermes/path/types.h"

namespace memoria::hermes::path { namespace parser {

/**
 * @brief The EncodeSurrogatePairAction class is a functor for encoding
 * surrogate pair characters in UTF-32.
 */
class EncodeSurrogatePairAction
{
public:
    /**
     * @brief The action's result type
     */
    using result_type =  UnicodeChar;
    /**
     * @brief Encodes a surrogate pair character
     * @param[in] highSurrogate High surrogate
     * @param[in] lowSurrogate Low surrogate
     * @return The result of @a highSurrogate and @a lowSurrogate combined
     * into a single codepoint
     */
    result_type operator()(UnicodeChar const& highSurrogate,
                           UnicodeChar const& lowSurrogate) const
    {
        UnicodeChar unicodeChar = 0x10000;
        unicodeChar += (highSurrogate & 0x03FF) << 10;
        unicodeChar += (lowSurrogate & 0x03FF);
        return unicodeChar;
    }
};
}} // namespace hermes::path::parser
#endif // ENCODESURROGATEPAIRACTION_H
