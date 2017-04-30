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


// Idiomatic Map<K, V> example for Memoria. Map is an ordered mapping between keys and values
// backed with b-tree like structure. Besides traditional key-based lookup, Map<> also supports
// entry lookup by entry index from the beginning and bulk insertion/removal.

// Not every object type is supported as a key or value. In short, custom value codec or field factory
// must be defined. Out of the box, all primitive types are supported, String, BigInteger and some
// others (not yet implemented ones). Fixed-size types are somewhat faster on updates than
// variable-length (String, BigInteger) ones.


#include <memoria/v1/allocators/inmem/threads/allocator_inmem_threads_api.hpp>
#include <memoria/v1/api/vector/vector_api.hpp>


using namespace memoria::v1;
using namespace std;

int main()
{
    using Value = int;

    try {
        auto alloc = ThreadInMemAllocator<>::create();

        auto snp = alloc.master().branch();
        
        auto vec = create<Vector<Value>>(snp);
        
        std::vector<Value> vec1(10000);
        vec.begin().insert(vec1);
        
        auto vv = vec.seek(0).read(12345);
        
        std::cout << vv.size() << std::endl;
        
        snp.commit();
        
        snp.dump("data.dir");
    }
    catch (Exception& ex)
    {
        cout << ex.message() << " at " << ex.source() << endl;
    }
}
