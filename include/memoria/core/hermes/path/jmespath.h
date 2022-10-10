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
#ifndef JMESPATH_H
#define JMESPATH_H
#include <string>
#include <memoria/core/hermes/path/types.h>
#include <memoria/core/hermes/path/exceptions.h>
#include <memoria/core/hermes/path/expression.h>

/**
 * @mainpage %jmespath.cpp
 *
 * %jmespath.cpp is a C++ implementation of <a href="http://jmespath.org">
 * JMESPath</a>, a query language for JSON.
 * It can be used to extract and transform elements of a JSON document.
 *
 * @section example JMESPath expression example
 *
 * Input JSON document:
 * @code{.javascript}
 * {
 *   "locations": [
 *     {"name": "Seattle", "state": "WA"},
 *     {"name": "New York", "state": "NY"},
 *     {"name": "Bellevue", "state": "WA"},
 *     {"name": "Olympia", "state": "WA"}
 *   ]
 * }
 * @endcode
 *
 * JMESPath expression:
 * @code{.javascript}
 * locations[?state == 'WA'].name | sort(@) | {WashingtonCities: join(', ', @)}
 * @endcode
 *
 * Result of evaluating the expression:
 * @code{.javascript}
 * {"WashingtonCities": "Bellevue, Olympia, Seattle"}
 * @endcode
 *
 * For more examples take a look at the
 * <a href="http://jmespath.org/tutorial.html">JMESPath Tutorial</a> or the
 * <a href="http://jmespath.org/examples.html">JMESPath Examples</a> pages.
 *
 * @section usage Using jmespath.cpp
 *
 * For installation instructions check out the @ref install page.
 *
 * To use the public functions and classes of %jmespath.cpp you should include
 * the header file `"#include <jmespath/jmespath.h>"`.
 * The public interface is declared in the `"jmespath"` namespace.
 * @code{.cpp}
 * #include <iostream>
 * #include <jmespath/jmespath.h>
 *
 * namespace jp = jmespath;
 *
 * int main(int argc, char *argv[])
 * {
 *     auto data = R"({
 *         "locations": [
 *             {"name": "Seattle", "state": "WA"},
 *             {"name": "New York", "state": "NY"},
 *             {"name": "Bellevue", "state": "WA"},
 *             {"name": "Olympia", "state": "WA"}
 *         ]
 *     })"_json;
 *     jp::Expression expression = "locations[?state == 'WA'].name | sort(@) | "
 *                                 "{WashingtonCities: join(', ', @)}";
 *     std::cout << jp::search(expression, data) << std::endl;
 *     return 0;
 * }
 * @endcode
 *
 * @subsection search Search function
 * The main entry point of the library is the @ref jmespath::search function
 * which takes a JMESPath expression as its first argument and a JSON document
 * as the second argument and returns the result of the evaluated expression
 * as a new JSON document.
 * @code{.cpp}
 * auto input = R"({"foo": "bar"})"_json;
 * auto result = jmespath::search("foo", input);
 * @endcode
 *
 * Besided passing the input document as an lvalue reference you can also pass
 * it as an rvalue reference, which would allow the library to take advantage
 * of move operations internally and produce the output document in a more
 * efficient and faster manner.
 * @code{.cpp}
 * auto input = R"({"foo": "bar"})"_json;
 * auto result = jmespath::search("foo", std::move(input));
 * @endcode
 *
 * @subsection expression Expression class
 * The @ref jmespath::Expression class allows to store a parsed JMESPath
 * expression which is usefull if you want to evaluate the same expression
 * on multiple JSON documents.
 * @code{.cpp}
 * jmespath::Expression expression {"foo"};
 * auto result1 = jmespath::search(expression, R"({"foo": "bar"})"_json);
 * auto result2 = jmespath::search(expression, R"({"foo": {"bar": "baz"}})"_json);
 * @endcode
 *
 * By importing the names from the @ref jmespath::literals namespace you can
 * also use user defined literals to define JMESPath expressions.
 * @code{.cpp}
 * using namespace jmespath::literals;
 *
 * auto expression = "foo"_jmespath;
 * auto result1 = jmespath::search(expression, R"({"foo": "bar"})"_json);
 * auto result2 = jmespath::search(expression, R"({"foo": {"bar": "baz"}})"_json);
 * @endcode
 *
 * @subsection json JSON documents
 * For the handling of JSON documents and values %jmespath.cpp relies on the
 * excelent <a href="https://github.com/nlohmann/json">nlohmann_json</a>
 * library.
 *
 * For defining JSON values you can either use `"_json"` user defined literal
 * or the @ref jmespath::Json type which is just an alias for
 * <a href="https://github.com/nlohmann/json">nlohmann_json</a>
 * @code{.cpp}
 * jmespath::Json jsonObject {{"foo", "bar"}};
 * @endcode
 *
 * or you can also include the header of the
 * <a href="https://github.com/nlohmann/json">nlohmann_json</a> library and use
 * the `"nlohmann::json"` type directly.
 * @code{.cpp}
 * #include <nlohmann/json.hpp>
 *
 * nlohmann::json jsonObject {{"foo", "bar"}};
 * @endcode
 *
 * @subsection error Error handling
 * All the exceptions that might get thrown by @ref jmespath::search or
 * @ref jmespath::Expression are listed on the @ref exceptions page.
 *
 * The library uses the
 * <a href="https://www.boost.org/doc/libs/1_69_0/libs/exception/doc/
 * boost-exception.html">Boost Exception</a> library for throwing exceptions.
 * Which means that the thrown exceptions can
 * <a href="https://www.boost.org/doc/libs/1_69_0/libs/exception/doc/
 * tutorial_transporting_data.html">carry additional data attached</a>
 * to them. All the additional data that exceptions might carry is listed on
 * the @ref error_info page.
 *
 * This additional information can be used for example to pinpoint exactly
 * which part of a JMESPath expression caused a parsing failure.
 * @code{.cpp}
 * jmespath::Expression expression;
 *
 * try
 * {
 *      expression = "foo?";
 * }
 * catch (jmespath::SyntaxError& error)
 * {
 *      if (const std::string* searchExpression
 *              = boost::get_error_info<jmespath::InfoSearchExpression>(error))
 *      {
 *          std::cerr << "Failed parsing expression: "
 *                    << *searchExpression << std::endl;
 *      }
 *      if (const long* location
 *              = boost::get_error_info<jmespath::InfoSyntaxErrorLocation>(
 *                  error))
 *      {
 *          std::cerr << "Error at position: " << *location << std::endl;
 *      }
 * }
 * @endcode
 * This code would produce the following output:
 * @code
 * Failed parsing expression: foo?
 * Error at position: 3
 * @endcode
 *
 * You can also use <a href="https://www.boost.org/doc/libs/1_69_0/libs/
 * exception/doc/tutorial_diagnostic_information.html">
 * boost::diagnostic_information</a> function to get all the additional
 * information attached to an exception.
 * @code{.cpp}
 * jmespath::Expression expression;
 *
 * try
 * {
 *      expression = "foo?";
 * }
 * catch (jmespath::SyntaxError& error)
 * {
 *      std::cerr << boost::diagnostic_information(error) << std::endl;
 * }
 * @endcode
 * This code would produce the following output:
 * @code
 * jmespath.cpp/src/parser/parser.h(90): Throw in function jmespath::parser::Parser::ResultType jmespath::parser::Parser<parser::Grammar>::parse(const jmespath::String &) [T = parser::Grammar]
 * Dynamic exception type: boost::exception_detail::clone_impl<jmespath::SyntaxError>
 * std::exception::what: std::exception
 * [jmespath::tag_search_expression*] = foo?
 * [jmespath::tag_syntax_error_location*] = 3
 * @endcode
 */

/**
 * @page install Installation and integration
 * @section requirements Requirements
 * To build, install and use the library you must have
 * <a href="https://cmake.org/">CMake</a> installed, version 3.8 or later.
 *
 * @subsection compilers Supported compilers
 * %jmespath.cpp needs a compiler that supports at least the c++14 standard.
 * The currently supported and tested compilers are:
 * - g++ versions: 6, 7, 8
 * - Clang versions: 4.0, 5.0, 6.0, 7
 * - XCode versions: 9.0, 9.3, 10.1
 * - Visual Studio 2017
 *
 * @subsection dependencies Library dependencies
 * - <a href="https://www.boost.org/">boost</a> version 1.65 or later
 * - <a href="https://github.com/nlohmann/json">nlohmann_json</a> version 3.4.0
 * or later
 *
 * @subsection install_from_source Install from source
 * @subsubsection build_install Build and install
 * To get the source code of the library either clone it from
 * <a href="https://github.com/robertmrk/jmespath.cpp">github</a>
 * @code{.bash}
 * git clone https://github.com/robertmrk/jmespath.cpp.git
 * @endcode
 * or download the <a href="https://github.com/robertmrk/jmespath.cpp/releases">
 * latest release</a> and extract it.
 *
 * In the terminal change the current directory to the location of the source
 * code
 * @code{.bash}
 * cd <path_to_source>/jmespath.cpp
 * @endcode
 * Generate the project or makefiles for the build system of your choice with
 * CMake, then build and install the library:
 * @code{.bash}
 * mkdir build
 * cd build
 * cmake .. -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DJMESPATH_BUILD_TESTS=OFF
 * sudo cmake --build . --target install
 * @endcode
 *
 * @subsubsection integration Integration
 * To use the library in your CMake project you should find the library with
 * `"find_package"` and link your target with `"jmespath::jmespath"`:
 * @code
 * cmake_minimum_required(VERSION 3.0)
 * project(example)
 *
 * find_package(jmespath 0.1.0 CONFIG REQUIRED)
 *
 * add_executable(${PROJECT_NAME} main.cpp)
 * target_link_libraries(${PROJECT_NAME} jmespath::jmespath)
 * target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_14)
 * @endcode
 *
 * @subsection conan Install with Conan
 * If you are using <a href="https://www.conan.io/">Conan</a> to manage your
 * dependencies, then add `%jmespath.cpp/x.y.z@robertmrk/stable` to your
 * conanfile.py's requires, where `x.y.z` is the release version you want to
 * use.
 * Please file issues
 * <a href="https://github.com/robertmrk/conan-jmespath.cpp/issues">here</a>
 * if you experience problems with the packages.
 */

/**
 * @defgroup public The public API of the library
 */

/**
 * @brief The top level namespace which contains the public
 * functions of the library
 */
namespace memoria {
namespace jmespath {

/**
 * @ingroup public
 * @brief Finds or creates the results for the @a expression evaluated on the
 * given @a document.
 *
 * The @a expression string should be encoded in UTF-8.
 * @param expression JMESPath expression.
 * @param document Input JSON document
 * @return Result of the evaluation of the @a expression in @ref Json format
 * @note This function is reentrant. Since it takes the @a expression by
 * reference the value of the @a expression should be protected from changes
 * until the function returns.
 * @throws InvalidAgrument If a precondition fails. Usually signals an internal
 * error.
 * @throws InvalidValue When an invalid value is specified for an *expression*.
 * For example a `0` step value for a slice expression.
 * @throws UnknownFunction When an unknown JMESPath function is called in the
 * *expression*.
 * @throws InvalidFunctionArgumentArity When a JMESPath function is called with
 * an unexpected number of arguments in the *expression*.
 * @throws InvalidFunctionArgumentType When an invalid type of argument was
 * specified for a JMESPath function call in the *expression*.
 */
template <typename JsonT>
std::enable_if_t<std::is_same<std::decay_t<JsonT>, Json>::value, Json>
search(const Expression& expression, JsonT&& document);

/**
 * @brief Explicit instantiation declaration for @ref search to prevent
 * implicit instantiation in client code.
* @{
*/
extern template Json search<const Json&>(const Expression&, const Json&);
extern template Json search<Json&>(const Expression&, Json&);
extern template Json search<Json>(const Expression&, Json&&);
/** @}*/
} // namespace jmespath
}

#endif // JMESPATH_H
