// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/memoria.hpp>
#include <memoria/containers/seq_dense/seqd_factory.hpp>

#include <memoria/core/tools/random.hpp>
#include <memoria/core/tools/time.hpp>

using namespace memoria;
using namespace std;

int main(int argc, const char** argv, const char** envp) {
	MEMORIA_INIT(DefaultProfile<>);

	try {
		SmallInMemAllocator alloc;

		alloc.mem_limit() = 2*1024*1024*1024ll;

		constexpr Int BitsPerSymbol = 1;

		using CtrT = DCtrTF<Sequence<BitsPerSymbol>>::Type;

		CtrT::initMetadata();

		CtrT ctr(&alloc);

//		ctr.setNewPageSize(32*1024);

		auto iter = ctr.seek(0);

		using Provider = seq_dense::RandomSequenceInputProvider<CtrT>;
		Provider provider(ctr, getGlobalBigIntGenerator(), 4000000, 1000000);

		ctr.insert(iter, provider);

		alloc.commit();

		cout<<"size: "<<ctr.size()<<endl;

		auto i1 = ctr.seek(ctr.size());

		BigInt total_rank = 0;

		for (Int s = 0; s < 1<<BitsPerSymbol; s++) {
			total_rank += i1.rank(s);
		}

		cout<<"iter rank0: "<<i1.rank(0)<<endl;
		cout<<"iter rank1: "<<i1.rank(1)<<endl;

		cout<<"Total Rank: "<<total_rank<<endl;

		auto size = ctr.size();

		cout<<"ctr rank0: "<<ctr.rank(0, size+1000, 0)<<endl;
		cout<<"ctr rank1: "<<ctr.rank(0, size+1000, 1)<<endl;


		auto ii = ctr.seek(size - 1);

		cout<<"ii rank0: "<<i1.rankBw(size - 1, 0)<<endl;

		cout<<"Allocated: "<<(alloc.allocated()/1024)<<"K"<<endl;

		auto r0 = ctr.rank(0, size, 0);
		cout<<"select0: "<<ctr.select(0, r0/2).pos()<<endl;

		if (argc > 1)
		{
			const char* dump_name = argv[1];

			cout<<"Dump to: "<<dump_name<<endl;

			OutputStreamHandler* os = FileOutputStreamHandler::create(dump_name);
			alloc.store(os);
			delete os;
		}

		cout<<"Done"<<endl;
	}
	catch (memoria::vapi::PackedOOMException& ex) {
		cout<<"PackedOOMException at "<<ex.source()<<endl;
	}
	catch (memoria::vapi::Exception& ex) {
		cout<<ex.message()<<" at "<<ex.source()<<endl;
	}
}
