
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


#include <memoria/profiles/default/default.hpp>
#include <memoria/core/datatypes/datatypes.hpp>

#include <memoria/core/datatypes/varchars/varchar_builder.hpp>
#include <memoria/core/datatypes/buffer/buffer.hpp>

#include <memoria/api/store/memory_store_api.hpp>

#include <memoria/api/vector/vector_api.hpp>

#include <memoria/memoria.hpp>

#include <memoria/core/tools/result.hpp>

#include <iostream>
#include <stdexcept>

using namespace memoria;

struct Resource {

    Resource(MaybeError& mabe_error, U8String name)
    {
        wrap_construction(mabe_error, [&]() {
            std::cout << "Initilizing Resource " << name << std::endl;

            //return make_generic_error("Resource {} graceful faiulre", name);

//            if (1==1) {
//                throw std::runtime_error("Resource " + name.to_std_string() + " failure");
//            }
        });
    }
};

struct Aggregate {

    Resource res1_;
    Resource res2_;

    Aggregate(MaybeError& mabe_error):
        res1_(mabe_error, "RES1"),
        res2_(mabe_error, "RES2")
    {
        wrap_construction(mabe_error, [](){
            std::cout << "Initilizing Aggregate" << std::endl;
        });
    }
};

Result<Aggregate> make_aggreagte() {
    MaybeError maybe_error;
    Aggregate agg(maybe_error);
    if (!maybe_error) {
        return Result<Aggregate>::of(std::move(agg));
    }
    else {
        return std::move(maybe_error.get());
    }
}

int main()
{
    try {
        Result<Aggregate> res = make_aggreagte();
        res.get_or_throw();
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
