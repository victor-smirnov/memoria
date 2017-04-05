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




#include <memoria/v1/memoria.hpp>
#include <memoria/v1/containers/set/set_factory.hpp>
#include <memoria/v1/core/tools/iobuffer/io_buffer.hpp>

#include <memoria/v1/core/tools/strings/string_codec.hpp>
#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/core/tools/fixed_array.hpp>
#include <memoria/v1/core/tools/random.hpp>
#include <memoria/v1/core/tools/ticker.hpp>

#include <algorithm>
#include <vector>
#include <type_traits>
#include <iostream>


using namespace memoria::v1;
using namespace memoria::v1::btss;
using namespace std;

template <typename> struct XCtr;
template <typename> struct XIter;

template <typename T> struct XIterBase {
	using MyType = XIter<T>;
	
	using CtrType = XCtr<T>;
	using IterType = XIter<T>;

	void foo() {
		std::cout << std::get<0>(self().ctr().template boo<1>(self().me())) << std::endl;
		self().boo();
	}

	MyType& self() { return *static_cast<MyType*>(this); }
};

template <typename T>
struct XIter : XIterBase<T> {
	using Base = XIterBase<T>;
	using typename Base::CtrType;
	using typename Base::IterType;

	CtrType* ctr_;
	CtrType& ctr() { return *ctr_; }

	XIter(CtrType* ctr) : ctr_(ctr) {}



	void boo() {
		std::cout << "Iter Bingo!" << std::endl;
	}

	IterType& me() { return *this; }

};


template <typename T> struct XBase {
	using MyType = XCtr<T>;
	using IterType = XIter<T>;

	void foo() 
	{
		self().boo();
	}

	MyType& self() { return *static_cast<MyType*>(this); }
};

template <typename T> 
struct XCtr : XBase<T> {
	using Base = XBase<T>;

	using typename Base::IterType;

	auto boo() {
		std::cout << "Bingo!" << std::endl;
		return std::make_tuple(1);
	}

	template <Int Idx>
	auto boo(IterType& ii) 
	{
		std::cout << "Ctr !!Bingo!!" << std::endl;

		ii.foo();

		return std::make_tuple(2);
	}
};





int main()
{
	XCtr<Int> cc;
	XIter<Int> itr(&cc);

	cc.foo();
	itr.foo();

	int ii;
	std::cin >> ii;

    MEMORIA_INIT(DefaultProfile<>);
	
    using Key   = FixedArray<16>;
    //using Key   = Bytes;


    DInit<Set<Key>>();
	
    try {
        auto alloc = PersistentInMemAllocator<>::create();
		/*
        auto snp = alloc->master()->branch();

        auto map = create<Set<Key>>(snp);
        map->setNewPageSize(65536);

        int size = 30000000;

        Ticker ticker(100000);

        BigInt t0 = getTimeInMillis();
        BigInt tl = t0;



        for (int c = 0; c < size; c++)
        {
            Key array;//(16);

            for (int c = 0; c < array.length(); c++)
            {
                array[c] = getRandomG(256);
            }

            map->insert_key(array);

            if (ticker.is_threshold())
            {
            	BigInt tt = getTimeInMillis();
            	cout << "Inserted: " << (ticker.ticks() + 1)<< " in " << (tt - tl) << endl;
            	tl = tt;

            	ticker.next();
            }

            ticker.tick();
        }

        BigInt t1 = getTimeInMillis();

        cout << "Inserted " << size << " in " << (t1 - t0) << endl;


        snp->commit();

//        FSDumpAllocator(snp, "setl_full.dir");

        // Store binary contents of allocator to the file.
        auto out = FileOutputStreamHandler::create("setl_data.dump");
        alloc->store(out.get());
		/**/
    }
    catch (Exception& ex)
    {
        cout << ex.message() << " at " << ex.source() << endl;
    }
	

    // Destroy containers metadata.
    MetadataRepository<DefaultProfile<>>::cleanup();

	/**/
}


