/****************************************************************************
**
** Author: Róbert Márki <gsmiko@gmail.com>
** Copyright (c) 2016 Róbert Márki
**
** This file is part of the jmespath.cpp project which is distributed under
** the MIT License (MIT).
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
#ifndef PARSER_H
#define PARSER_H


#define BOOST_SPIRIT_UNICODE

#include "memoria/core/hermes/path/types.h"
#include "memoria/core/hermes/path/exceptions.h"
#include <boost/spirit/include/qi.hpp>

namespace memoria::hermes::path { namespace parser {

namespace qi = boost::spirit::qi;
namespace encoding = qi::unicode;

/**
 * @brief The Parser class parses expressions with the specified grammar.
 * @tparam T Grammar type to use for parsing
 */
template <template<typename, typename> class T>
class Parser
{
public:
    /**
     * @brief Iterator type which will be used to instantiate the grammar
     */
    using IteratorType  = UnicodeIteratorAdaptor;
    /**
     * @brief The type of the grammar that will be instantiated
     */
    using GrammarType   = T<IteratorType, encoding::space_type>;
    /**
     * @brief The type of the result of parsing
     */
    using ResultType    = typename GrammarType::start_type::attr_type;

    /**
     * @brief Parses the given @a expression
     * @param[in] expression JMESPath search expression encoded in UTF-8
     * @return The result of parsing the expression. The result's type is the
     * same as the attribute type of the start rule in the specified grammar.
     * @throws SyntaxError
     */
    ResultType parse(const String& expression)
    {
        try
        {
            // create begin and end iterators to the expression
            // retain the value of the begin iterator
            IteratorType beginIt(expression.cbegin());
            IteratorType it = beginIt;
            IteratorType endIt(expression.cend());

            ResultType result;
            // parse JMESPath expression and create the result
            bool parsingSuccesful = qi::phrase_parse(it, endIt,
                                                     m_grammar, encoding::space,
                                                     result);
            // if parsing wasn't sucessful or if the expression was
            // only partially parsed
            if (!parsingSuccesful || (it != endIt))
            {
                // throw exception and set the location of syntax error
                auto syntaxErrorLocation = std::distance(beginIt, it);
                auto exception = SyntaxError();
                exception << InfoSyntaxErrorLocation(syntaxErrorLocation);
                BOOST_THROW_EXCEPTION(exception);
            }
            return result;
        }
        catch(Exception& exception)
        {
            exception << InfoSearchExpression(expression);
            throw;
        }
    }

private:
    /**
     * @brief Grammar object used for parsing
     */
    GrammarType m_grammar;
};
}} // namespace hermes::path::parser
#endif // PARSER_H
