
// Copyright Victor Smirnov 2011+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINER_vctr_ITERATOR_API_HPP
#define _MEMORIA_CONTAINER_vctr_ITERATOR_API_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/idata.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/containers/vector/vctr_names.hpp>
#include <memoria/containers/vector/vctr_tools.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/bt_macros.hpp>

#include <iostream>

namespace memoria    {

MEMORIA_ITERATOR_PART_BEGIN(memoria::mvector::ItrApiName)

    typedef Ctr<typename Types::CtrTypes>                                       Container;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBase                                             NodeBase;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Container::Value                                           Value;
    typedef typename Container::Accumulator                                     Accumulator;

    typedef typename Container::Types::Pages::LeafDispatcher                    LeafDispatcher;
    typedef typename Container::Position                                        Position;

    using CtrSizeT = typename Container::Types::CtrSizeT;

    using InputBuffer = typename Container::Types::InputBuffer;


    void insert(std::vector<Value>& data)
    {
        auto& self = this->self();
        auto& model = self.ctr();

        auto& leaf = self.leaf();

        mvector::StdVectorInputBuffer<InputBuffer> buf(data);

        auto result = model.insertBuffers(leaf, self.idx(), buf.me());

        self.leaf() = result.leaf();
        self.idx() 	= result.position();

        self.refreshCache();

        model.markCtrUpdated();
    }

    void insert(Value data)
    {
        auto& self = this->self();
        auto& model = self.ctr();

        MemBuffer<Value> buf(&data, 1);

        model.insert(self, buf);

        model.markCtrUpdated();
    }



    struct VectorReadWalker {
    	std::vector<Value>& data_;
    	CtrSizeT processed_ = 0;
    	const CtrSizeT max_;

    	VectorReadWalker(std::vector<Value>& data): data_(data), max_(data.size()) {}

    	template <typename NodeTypes>
    	Int treeNode(const LeafNode<NodeTypes>* leaf, Int start)
    	{
    		return std::get<0>(leaf->template processSubstreams<IntList<0>>(*this, start));
    	}

    	template <typename StreamObj>
    	Int stream(const StreamObj* obj, Int start)
    	{
    		if (obj != nullptr)
    		{
    			Int size 		= obj->size();
    			Int remainder 	= size - start;

    			Int to_read = (processed_ + remainder < max_) ? remainder : (max_ - processed_);

    			auto i = processed_;

    			obj->read(0, start, start + to_read, [&](Value v){
    				data_[i++] = v;
    			});

    			return to_read;
    		}
    		else {
    			return 0;
    		}
    	}


    	void start_leaf() {}

    	void end_leaf(Int skip) {
    		processed_ += skip;
    	}

    	CtrSizeT result() const {
    		return processed_;
    	}

    	bool stop() const {
    		return processed_ >= max_;
    	}
    };



    CtrSizeT read(std::vector<Value>& data)
    {
    	auto& self = this->self();

    	VectorReadWalker walker(data);

    	return self.ctr().readStream2(self, walker);
    }


    Value value() const
    {
        MyType me = this->self();

        Value data;
        MemBuffer<Value> buf(&data, 1);

        CtrSizeT length = me.read(buf);

        if (length == 1)
        {
            return data;
        }
        else if (length == 0)
        {
            throw Exception(MA_SRC, "Attempt to read vector after its end");
        }
        else {
            throw Exception(MA_SRC, "Invalid vector read");
        }
    }

//    void remove(CtrSizeT size)
//    {
//        auto& self = this->self();
//        self.ctr().remove(self, size);
//
//        self.ctr().markCtrUpdated();
//    }

    std::vector<Value> subVector(CtrSizeT size)
    {
        std::vector<Value> data(size);

        auto iter = self();

        auto readed = iter.read(data);

        MEMORIA_ASSERT(readed, ==, size);

        return data;
    }

    auto skipFw(CtrSizeT amount) {
    	return self().template _skipFw<0>(amount);
    }

    auto skipBw(CtrSizeT amount) {
    	return self().template _skipBw<0>(amount);
    }

    auto skip(CtrSizeT amount) {
    	return self().template _skip<0>(amount);
    }

    auto seek(CtrSizeT pos)
    {
        CtrSizeT current_pos = self().pos();
        self().skip(pos - current_pos);
    }

    struct PosFn {
        Accumulator prefix_;

        template <typename NodeTypes>
        void treeNode(const LeafNode<NodeTypes>* node, Int idx) {}

        template <typename NodeTypes>
        void treeNode(const BranchNode<NodeTypes>* node, Int idx)
        {
            node->sums(0, idx, prefix_);
        }
    };


    CtrSizeT pos() const
    {
        auto& self = this->self();

        PosFn fn;

        self.ctr().walkUp(self.leaf(), self.idx(), fn);

        return std::get<0>(fn.prefix_)[0] + self.key_idx();
    }

    CtrSizeT prefix() const
    {
        return self().cache().size_prefix()[0];
    }

    Accumulator prefixes() const {
        Accumulator acc;
        std::get<0>(acc)[0] = prefix();
        return acc;
    }

    void ComputePrefix(CtrSizeT& accum)
    {
        accum = prefix();
    }

    void ComputePrefix(Accumulator& accum)
    {
        accum = prefixes();
    }

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::mvector::ItrApiName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS



}

#undef M_TYPE
#undef M_PARAMS

#endif
