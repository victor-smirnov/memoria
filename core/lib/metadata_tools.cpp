
// Copyright 2011-2023 Victor Smirnov
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

#include <memoria/profiles/common/block_operations.hpp>
#include <memoria/core/tools/dump.hpp>

#include <sstream>
#include <string>
#include <ostream>

namespace memoria {


void Expand(std::ostream& os, size_t level)
{
    for (size_t c = 0; c < level; c++) {
        os<<" ";
    }
}


size_t max_width(const BlockDataValueProvider& provider)
{
    size_t max = 0;

    for (size_t c = 0; c < provider.size(); c++)
    {
        auto str = provider.value(c);

        auto len = str.length();

        if (len > max)
        {
            max = len;
        }
    }

    return max;
}


void dumpPageDataValueProviderAsArray(std::ostream& out, const BlockDataValueProvider& provider)
{
    auto width = max_width(provider) + 1;

    if (width < 3) width = 3;

    size_t columns;

    if (width <= 3) {
        columns = 16;
    }
    else if (width <= 7) {
        columns = 16;
    }
    else {
        columns = (80 / width > 0 ? 80 / width : 1);
    }

    out << std::endl;
    Expand(out, 28);
    for (size_t c = 0; c < columns; c++)
    {
        out.width(width);
        out << c;
    }

    out << std::dec << std::endl;

    for (size_t c = 0; c < provider.size(); c+= columns)
    {
        Expand(out, 12);
        out << " ";
        out.width(6);
        out << std::dec << c << " " << std::hex;
        out.width(6);
        out << c << ": ";

        size_t d;
        for (d = 0; d < columns && c + d < provider.size(); d++)
        {
            std::stringstream ss;

            ss << provider.value(c + d);

            out.width(width);
            out<<ss.str();
        }

        out << std::dec << std::endl;
    }
}


}
