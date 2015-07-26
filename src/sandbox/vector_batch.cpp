// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/memoria.hpp>
#include <memoria/containers/mapx/mapx_factory.hpp>
#include <memoria/containers/seq_dense/seqd_factory.hpp>
#include <memoria/containers/vector/vctr_factory.hpp>

#include <memoria/core/container/metadata_repository.hpp>


using namespace memoria;
using namespace std;

std::uniform_int_distribution<int>      distribution;
std::mt19937_64                         engine;
auto                                    generator               = std::bind(distribution, engine);

template <typename CtrT>
class RandomVectorInputProvider: public memoria::btss::AbstractBTSSInputProvider<CtrT> {
	using Base = memoria::btss::AbstractBTSSInputProvider<CtrT>;

public:

	using Buffer 	= typename Base::Buffer;
	using CtrSizeT	= typename Base::CtrSizeT;
	using Position	= typename Base::Position;

	using Value = typename CtrT::Types::Value;

	using InputTuple 		= typename CtrT::Types::template StreamInputTuple<0>;
	using InputTupleAdapter = typename CtrT::Types::template InputTupleAdapter<0>;

	CtrSizeT bsize_;
	CtrSizeT pos_ = 0;

	Value max_;

	Value curr_ = 0;

public:
	RandomVectorInputProvider(CtrT& ctr, CtrSizeT size, Value max = 100, const Position& capacity = Position(10000)):
		Base(ctr, capacity),
		bsize_(size),
		max_(max)
	{}

	virtual bool get(InputTuple& value)
	{
		if (pos_ < bsize_)
		{
			value = InputTupleAdapter::convert(curr_++);

			pos_++;

			if (curr_ == max_) curr_ = 0;

			return true;
		}
		else {
			return false;
		}
	}
};




int main() {
	MEMORIA_INIT(SmallProfile<>);

	try {
		SmallInMemAllocator alloc;

		alloc.mem_limit() = 1024*1024*1024;

		using CtrT  = SCtrTF<Vector<Int>>::Type;

		CtrT::initMetadata();

		CtrT ctr(&alloc);

		auto iter = ctr.seek(0);

		using Position = RandomVectorInputProvider<CtrT>::Position;

		RandomVectorInputProvider<CtrT> provider(ctr, 100000000, 100, Position(10000));

		using Position = RandomVectorInputProvider<CtrT>::Position;

		ctr.insertData(iter.leaf(), Position(), provider);

		cout<<"data created"<<endl;

		alloc.commit();

//		OutputStreamHandler* os = FileOutputStreamHandler::create("vector_b.dump");
//		alloc.store(os);
//		delete os;
	}
	catch (memoria::vapi::Exception& ex) {
		cout<<ex.message()<<" at "<<ex.source()<<endl;
	}
}
