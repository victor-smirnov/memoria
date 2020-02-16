
// Copyright 2019 Victor Smirnov
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


#include <memoria/core/datatypes/datatypes.hpp>
#include <memoria/core/tools/result.hpp>

#include <iostream>
#include <stdexcept>
#include <type_traits>

using namespace memoria;

template <typename Fn, typename... Args>
auto function(Fn&& fn, Args&&... args) -> typename detail::ResultOfFn<decltype(fn(std::forward<Args>(args)...))>::Type {
    return wrap_throwing([&](){
        return fn(std::forward<Args>(args)...);
    });
}


int main()
{
    try {
        const int x = 0;

        function([&](auto&& a){
            return a;
        }, x).get_or_throw();

    }
    catch (MemoriaThrowable& th) {
        th.dump(std::cout);
    }
    catch (std::exception& ex) {
        std::cout << "std::exception has been thrown: " << ex.what() << std::endl;
    }
    catch (int& vv) {
        std::cout << "Int has been thrown: " << vv << std::endl;
    }

    return 0;
}
