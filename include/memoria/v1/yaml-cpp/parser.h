
// Copyright (c) 2008-2015 Jesse Beder.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include <ios>
#include <memory>

#include <memoria/v1/yaml-cpp/dll.h>
#include <memoria/v1/yaml-cpp/noncopyable.h>

namespace memoria {
namespace v1 {
namespace YAML {
class EventHandler;
class Node;
class Scanner;
struct Directives;
struct Token;

/**
 * A parser turns a stream of bytes into one stream of "events" per YAML
 * document in the input stream.
 */
class YAML_CPP_API Parser : private noncopyable {
 public:
  /** Constructs an empty parser (with no input. */
  Parser();

  /**
   * Constructs a parser from the given input stream. The input stream must
   * live as long as the parser.
   */
  explicit Parser(std::istream& in);

  ~Parser();

  /** Evaluates to true if the parser has some valid input to be read. */
  explicit operator bool() const;

  /**
   * Resets the parser with the given input stream. Any existing state is
   * erased.
   */
  void Load(std::istream& in);

  /**
   * Handles the next document by calling events on the {@code eventHandler}.
   *
   * @throw a ParserException on error.
   * @return false if there are no more documents
   */
  bool HandleNextDocument(EventHandler& eventHandler);

  void PrintTokens(std::ostream& out);

 private:
  /**
   * Reads any directives that are next in the queue, setting the internal
   * {@code m_pDirectives} state.
   */
  void ParseDirectives();

  void HandleDirective(const Token& token);

  /**
   * Handles a "YAML" directive, which should be of the form 'major.minor' (like
   * a version number).
   */
  void HandleYamlDirective(const Token& token);

  /**
   * Handles a "TAG" directive, which should be of the form 'handle prefix',
   * where 'handle' is converted to 'prefix' in the file.
   */
  void HandleTagDirective(const Token& token);

 private:
  std::unique_ptr<Scanner> m_pScanner;
  std::unique_ptr<Directives> m_pDirectives;
};
}}}

