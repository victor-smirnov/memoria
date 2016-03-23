
// Copyright 2011 Victor Smirnov
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

#include <memoria/v1/core/exceptions/memoria.hpp>

namespace memoria {
namespace v1 {

const char* ExtractMemoriaPath(const char* path) {

    const char* prefix = MEMORIA_TOSTRING(MEMORIA_SRC);

    Int c;
    for (c = 0; prefix[c] != '\0'; c++)
    {
        if (prefix[c] != path[c] && (path[c] != '\\' && path[c] !='/'))
        {
            return path;
        }
    }

    return path + c;
}

ostream& operator<<(ostream& out, const MemoriaThrowable& t) {
    t.dump(out);
    return out;
}


}}
