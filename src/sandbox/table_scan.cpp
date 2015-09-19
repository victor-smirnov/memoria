// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/memoria.hpp>
#include <memoria/containers/mapx/mapx_factory.hpp>
#include <memoria/containers/table/table_factory.hpp>
#include <memoria/containers/seq_dense/seqd_factory.hpp>
#include <memoria/containers/vector/vctr_factory.hpp>

#include <memoria/core/container/metadata_repository.hpp>

#include <memoria/core/tools/time.hpp>

std::uniform_int_distribution<int>      distribution;
std::mt19937_64                         engine;
auto                                    generator               = std::bind(distribution, engine);


using namespace memoria;
using namespace memoria::tools;
using namespace std;

using CtrT 		= SCtrTF<Table<BigInt, Byte>>::Type;
using Provider 	= table::RandomDataInputProvider<CtrT, decltype(generator)>;
using Position  = Provider::Position;


struct ScanFn {
	BigInt value_ = 0;

	template <typename Stream>
	void operator()(const Stream* obj, Int start, Int end)
	{
		value_++;
	}
};


int main(int argc, const char** argv, const char** envp) {
	MEMORIA_INIT(SmallProfile<>);

	try {
		SmallInMemAllocator alloc;

		alloc.mem_limit() = 2*1024*1024*1024ll;

		CtrT::initMetadata();

		CtrT ctr(&alloc);

		auto iter = ctr.seek(0);

		Int rows 		= 200;
		Int cols		= 10;
		Int data_size	= 100;

		BigInt c0 = getTimeInMillis();

		Provider provider(ctr, rows + 1, cols, data_size, generator);

		ctr.insertData(iter.leaf(), Position(), provider);

		BigInt c1 = getTimeInMillis();

		cout<<"Table Constructed in "<<FormatTime(c1 - c0)<<" s"<<endl;

		alloc.commit();

		if (argc > 1)
		{
			const char* dump_name = argv[1];

			cout<<"Dump to: "<<dump_name<<endl;

			OutputStreamHandler* os = FileOutputStreamHandler::create(dump_name);
			alloc.store(os);
			delete os;
		}

		ScanFn scan_fn;

		BigInt t0 = getTimeInMillis();

		for (int x = 0; x < 5; x++)
		{
			BigInt tt0 = getTimeInMillis();

			iter = ctr.seek(0);
			for (Int r = 0; r < rows; r++)
			{
				MEMORIA_ASSERT(iter.pos(), ==, r);
				MEMORIA_ASSERT(iter.cache().abs_pos()[0], ==, r);
				MEMORIA_ASSERT(iter.size(), ==, rows + 1);

				if (r == 15) {
					DebugCounter = 1;
					iter.dump();
				}

				iter.toData();
				iter.checkPrefix();


				for (Int c = 0; c < cols; c++)
				{
					if (iter.size() != cols) {
						iter.dumpHeader();
					}

					MEMORIA_ASSERT(iter.pos(), ==, c);
					MEMORIA_ASSERT(iter.size(), ==, cols);

					iter.toData();

					iter.template scan<IntList<2>>(scan_fn);
					MEMORIA_ASSERT_TRUE(iter.isSEnd());

					iter.toIndex();
					iter.skipFw(1);

					if (c == cols){
						MEMORIA_ASSERT_TRUE(iter.isSEnd());
					}
				}

				if (r == 15) {
					iter.dumpHeader();
				}

				iter.toIndex();

				if (r == 15) {
					iter.dumpHeader();
				}

				iter.skipFw(1);

				if (r == 15) {
					iter.dumpHeader();
				}
			}

			BigInt tt1 = getTimeInMillis();

			cout<<"One Scan finished in "<<FormatTime(tt1 - tt0)<<endl;
		}

		BigInt t1 = getTimeInMillis();

		cout<<"All Scans finished in "<<FormatTime(t1 - t0)<<endl;

		cout<<"Done"<<endl;
	}
	catch (memoria::vapi::Exception& ex) {
		cout<<ex.message()<<" at "<<ex.source()<<endl;
	}
}
