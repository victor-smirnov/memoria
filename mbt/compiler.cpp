
// Copyright 2023 Victor Smirnov
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

#include "compiler.hpp"

namespace memoria::codegen {

class ClangCompilerConfig final: public CompilerConfig {
public:
    ClangCompilerConfig() {}

    std::vector<U8String> includes() {
        std::vector<U8String> paths;
        // TODO: add clang resource folder here
        return paths;
    }
};


std::shared_ptr<CompilerConfig> get_compiler_config() {
    return std::make_shared<ClangCompilerConfig>();
}

}
