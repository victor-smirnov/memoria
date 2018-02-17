
// Copyright 2018 Victor Smirnov
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


#include <memoria/v1/core/regexp/icu_regexp.hpp>

#include <iostream>

using namespace memoria::v1;

int main() {

    auto pattern = ICURegexPattern::compile(as_cu16_provider(u"a+"));
    std::cout << U8String(pattern.pattern()) << std::endl;

    auto matcher = pattern.matcher(as_cu16_provider("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), 20);

    std::cout << matcher.matches() << std::endl;
    std::cout << matcher.start() << std::endl;
    std::cout << matcher.end() << std::endl;

    std::cout << "'" << U16String(u" z   A   x ").trim() << "'" << std::endl;

    U16String str0(u"    The quick brown fox jumps over the lazy dog    ");

    std::cout << str0.find("dog") << std::endl;

    auto tokens = ICURegexPattern::compile(u" +").split(str0);

    for (auto& tt: tokens) {
        std::cout << tt << std::endl;
    }

    return 0;
}
