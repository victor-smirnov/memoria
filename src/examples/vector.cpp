// Copyright 2016 Victor Smirnov
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

#include <memoria/v1/profiles/default/default.hpp>
#include <memoria/v1/api/allocator/allocator_inmem_threads_api.hpp>
#include <memoria/v1/api/vector/vector_api.hpp>


using namespace memoria::v1;

int main()
{
    using Value = U8String;

    try {
        auto alloc = ThreadInMemAllocator<>::create();

        auto snp = alloc.master().branch();
        
        auto vec = create<Vector<Value>>(snp);
        
        std::vector<Value> vec1;
        
        for (int c = 0; c < 1000; c++) {
            vec1.emplace_back("str_" + std::to_string(c));
            //vec1.emplace_back(c);
        }
        
        //vec.begin().insert(vec1.begin(), vec1.end());
        size_t cnt = 0;
        vec.begin().insert_fn(vec1.size(), [&]{
            return vec1[cnt++];
        });
        
        auto vv = vec.seek(0).read(12345);
        
        std::cout << vv.size() << std::endl;
        
        snp.commit();
    }
    catch (MemoriaThrowable& ex)
    {
        ex.dump(std::cout);
    }
}
