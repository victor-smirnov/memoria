
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

#include <memoria/core/strings/format.hpp>

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>


#include <llvm/Support/raw_ostream.h>

#include <vector>

namespace memoria {
namespace codegen {

using namespace memoria::ld;

U8String describe_decl(const clang::Decl* decl) {
    std::string ss;
    llvm::raw_string_ostream os(ss);
    decl->print(os, 2);
    return U8String{ss};
}

std::vector<U8String> get_annotations(const clang::Decl* decl)
{
    std::vector<U8String> anns;

    if (decl->hasAttrs()) {
        for (const auto* attr: decl->getAttrs()) {
            if (attr->getKind() == clang::attr::Kind::Annotate) {
                const clang::AnnotateAttr* ann = clang::cast<const clang::AnnotateAttr>(attr);
                anns.push_back(ann->getAnnotation().str());
            }
        }
    }

    return anns;
}

}}
