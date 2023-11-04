
// Copyright 2021 Victor Smirnov
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

#include <memoria/core/strings/format.hpp>

#include <memory>
#include <string>

namespace clang {

class ASTUnit;
class Decl;
class Type;

}

namespace memoria {
namespace codegen {

template <typename T>
using ShPtr = std::shared_ptr<T>;

template <typename T>
using WeakPtr = std::weak_ptr<T>;

enum class InferenceType{
    VALUE, TYPE
};

class Inference {
    InferenceType itype_;
    U8String expression_;
public:
    Inference(U8String expression) noexcept:
        itype_(InferenceType::TYPE),
        expression_(expression)
    {}

    Inference(InferenceType itype, U8String expression) noexcept:
        itype_(itype),
        expression_(expression)
    {}

    InferenceType type() const noexcept {return itype_;}
    const U8String& expression() const noexcept {return expression_;}

    static Inference as_type(U8String expression) {
        return Inference(InferenceType::VALUE, expression);
    }

    static Inference as_value(U8String expression) {
        return Inference(InferenceType::VALUE, expression);
    }
};

struct CodeModule {
    virtual ~CodeModule() noexcept = default;

    virtual U8String file_path() const = 0;

    virtual clang::ASTUnit& ast_unit() noexcept = 0;
    virtual const clang::ASTUnit& ast_unit() const noexcept = 0;

    //virtual ShPtr<CodeModule> compile_with(U8String code) const = 0;
    //virtual std::vector<U8String> infer(const std::vector<Inference>& inferences) const = 0;
};

struct PreCompiledHeader {
    virtual ~PreCompiledHeader() noexcept = default;

    virtual ShPtr<PreCompiledHeader> parent() const noexcept = 0;

    virtual const std::vector<U8String>& options() const noexcept = 0;
    virtual U8String file_path() const = 0;
    virtual U8String target_folder() const = 0;

    virtual ShPtr<CodeModule> compile_with(U8String code) const = 0;
    virtual std::vector<U8String> infer(const std::vector<Inference>& inferences) const = 0;

    static ShPtr<PreCompiledHeader> create(
            const std::vector<U8String>& opts,
            const U8String& target_folder,
            const U8String& header_name
    );

    static ShPtr<PreCompiledHeader> create(
            ShPtr<PreCompiledHeader> parent,
            const U8String& target_folder,
            const U8String& header_name
    );
};


ShPtr<CodeModule> parse_file(const std::vector<U8String>& extra_opts, const U8String& file_name, const U8String& text);
ShPtr<CodeModule> parse_file(const std::vector<U8String>& extra_opts, const U8String& file_name);

U8String generate_pch(const U8String& target_directory, const U8String& header_file_name);


U8String regenerate_pch(const std::vector<U8String>& extra_opts, const U8String& target_directory, const U8String& header_file_name);

U8String regenerate_pch(const std::vector<U8String>& extra_opts, const U8String& target_directory, const U8String& pch, const U8String& header_file_name);

void add_parser_clang_option(const U8String& text);
const std::vector<std::string>& get_parser_options();

U8String load_text_file(U8String file_name);
void write_text_file(U8String file_name, U8String data);

void write_text_file_if_different(U8String file_name, U8String data);


}}
