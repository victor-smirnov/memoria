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
#ifndef APPENDUTF8ACTION_H
#define APPENDUTF8ACTION_H
#include "memoria/core/hermes/path/types.h"

namespace memoria::hermes::path { namespace parser {

/**
 * @brief The AppendUtf8Action class is a functor for appending UTF-32
 * characters to UTF-8 encoded strings.
 */
class AppendUtf8Action
{
public:
    /**
     * @brief The action's result type
     */
    using result_type = void;
    /**
    * @brief Appends the @a utf32Char character to the @a utf8String encoded in
    * UTF-8.
    * @param[out] utf8String The string where the encoded value of the
    * @a utf32Char will be appended.
    * @param[in] utf32Char The input character encoded in UTF-32
    */
    result_type operator()(String& utf8String,
                           UnicodeChar utf32Char) const
    {
        auto outIt = std::back_inserter(utf8String);
        boost::utf8_output_iterator<decltype(outIt)> utf8OutIt(outIt);
        *utf8OutIt++ = utf32Char;
    }
};
}} // namespace hermes::path::parser
#endif // APPENDUTF8ACTION_H
