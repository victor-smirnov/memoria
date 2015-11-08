
// Copyright Victor Smirnov 2011+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINER_vctr_ITERATOR_API_HPP
#define _MEMORIA_CONTAINER_vctr_ITERATOR_API_HPP

#include <memoria/core/types/types.hpp>
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


    CtrSizeT insert_v(const std::vector<Value>& data)
    {
        auto& self = this->self();

        mvector::IteratorVectorInputProvider<Container, typename std::vector<Value>::const_iterator> provider(self.ctr(), data.begin(), data.end());

        return self.insert(provider);
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



//    CtrSizeT read(std::vector<Value>& data)
//    {
//    	auto& self = this->self();
//
//    	VectorReadWalker walker(data);
//
//    	return self.ctr().readStream2(self, walker);
//    }


    Value value() const
    {
        auto me = this->self();

        auto v = me.subVector(1);

        if (v.size() == 1)
        {
            return v[0];
        }
        else if (v.size() == 0)
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
    	auto& self = this->self();

    	auto pos = self.pos();
    	auto ctr_size = self.size();

    	auto length = pos + size <= ctr_size ? size : ctr_size - pos;

        std::vector<Value> data(length);

        auto iter = self();

        auto begin = data.begin();

        auto readed = iter.ctr().read_entries(iter, size, [&](const auto& entry) {
        	*begin = std::get<0>(entry);
        	begin++;
        });

        MEMORIA_ASSERT(readed, ==, size);

        return data;
    }

    auto skipFw(CtrSizeT amount) {
    	return self().template skip_fw_<0>(amount);
    }

    auto skipBw(CtrSizeT amount) {
    	return self().template skip_bw_<0>(amount);
    }

    auto skip(CtrSizeT amount) {
    	return self().template skip_<0>(amount);
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
