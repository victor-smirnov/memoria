// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/v1/memoria.hpp>
#include <memoria/v1/containers/vector/vctr_factory.hpp>

#include <memoria/v1/core/container/metadata_repository.hpp>

#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/core/tools/random.hpp>

using namespace memoria;
using namespace v1::tools;
using namespace std;

template <typename CtrT>
class InputProvider2: public v1::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength> {
    using Base = v1::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength>;

public:
    using CtrSizeT  = typename Base::CtrSizeT;
    using Position  = typename Base::Position;
    using InputBuffer = typename Base::InputBuffer;

    using Value = typename CtrT::Types::Value;

    using InputTuple        = typename CtrT::Types::template StreamInputTuple<0>;
    using InputTupleAdapter = typename CtrT::Types::template InputTupleAdapter<0>;

    BigInt size_;
    BigInt current_ = 0;

    Int value_;

public:
    InputProvider2(CtrT& ctr, BigInt size, Int capacity = 10000):
        Base(ctr, capacity),
        size_(size)
    {}

    virtual Int get(InputBuffer* buffer, Int pos)
    {
        if (current_ < size_)
        {
            auto inserted = buffer->append(std::make_tuple(core::StaticVector<Int, 1>{value_}));

            current_+= inserted;

            return inserted;
        }

        return -1;
    }

    virtual void start_buffer(InputBuffer* buffer) {
        value_ = getRandomG(100);
    }
};


template <typename CtrT, typename InputIterator>
class IteratorInputProvider: public v1::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength> {
    using Base = v1::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength>;

public:
    using CtrSizeT  = typename Base::CtrSizeT;
    using Position  = typename Base::Position;
    using InputBuffer = typename Base::InputBuffer;

    using InputTuple        = typename CtrT::Types::template StreamInputTuple<0>;
    using InputTupleAdapter = typename CtrT::Types::template InputTupleAdapter<0>;

    using SV = core::StaticVector<typename InputIterator::value_type, 1>;

    using InputValue = std::tuple<SV>;

    InputIterator current_;
    InputIterator end_;



    Int input_start_ = 0;
    Int input_size_ = 0;
    static constexpr Int INPUT_END = 1000;

    InputValue input_value_buffer_[INPUT_END];

public:
    IteratorInputProvider(CtrT& ctr, InputIterator start, InputIterator end, Int capacity = 10000):
        Base(ctr, capacity),
        current_(start),
        end_(end)
    {
//      for (auto& v: input_value_buffer_) v = 0;
    }

    virtual Int get(InputBuffer* buffer, Int pos)
    {
        if (input_start_ == input_size_)
        {
            input_start_ = 0;

            for (input_size_ = 0 ;current_ != end_ && input_size_ < INPUT_END; input_size_++, current_++)
            {
                input_value_buffer_[input_size_] = std::make_tuple(SV(*current_));
            }
        }

        if (input_start_ < input_size_)
        {
            auto inserted = buffer->append(input_value_buffer_, input_start_, input_size_ - input_start_);

            input_start_ += inserted;

            return inserted;
        }

        return -1;
    }
};




using Buffer    = std::vector<Int>;

using CtrT              = DCtrTF<Vector<Int>>::Type;
using Provider          = InputProvider2<CtrT>;
using IteratorProvider  = IteratorInputProvider<CtrT, Buffer::const_iterator>;
using Position          = CtrT::Types::Position;




int main(int argc, const char** argv, const char** envp) {
    MEMORIA_INIT(DefaultProfile<>);

    try {
        SmallInMemAllocator alloc;

        alloc.mem_limit() = 2*1024*1024*1024ll;

        CtrT::initMetadata();

        CtrT ctr(&alloc);

//      ctr.setNewPageSize(16*4096);

        Buffer buffer(100000000);
        int cc = 0;
        for (auto& v: buffer) {
            v = cc++;
            if (cc == 1000) cc = 0;
        }

//      Provider provider(ctr, 100000000);
        IteratorProvider provider(ctr, buffer.begin(), buffer.end());

        long t0 = getTimeInMillis();

        ctr.seek(0).insert(provider);

        long t1 = getTimeInMillis();

        cout<<"Size: "<<ctr.size()<<" -- time: "<<(FormatTime(t1 - t0))<<" match: "<<(ctr.size() == buffer.size())<<endl;

        cout<<"Done"<<endl;
    }
    catch (v1::Exception& ex) {
        cout<<ex.message()<<" at "<<ex.source()<<endl;
    }
}
