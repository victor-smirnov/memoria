
// Copyright Victor Smirnov 2011+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQD_ITERATOR_MISC_HPP
#define _MEMORIA_CONTAINERS_SEQD_ITERATOR_MISC_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/containers/seq_dense/seqd_names.hpp>
#include <memoria/containers/seq_dense/seqd_tools.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/bt_macros.hpp>

#include <memoria/core/packed/array/packed_fse_bitmap.hpp>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::seq_dense::IterMiscName)

    typedef Ctr<typename Types::CtrTypes>                                       Container;


    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Container::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Container::Iterator                                     	Iterator;

    typedef typename Container::Types::Pages::LeafDispatcher                    LeafDispatcher;
    typedef typename Container::Position                                        Position;


    using CtrSizeT = typename Container::Types::CtrSizeT;

    template <Int Stream>
    using InputTupleAdapter = typename Container::Types::template InputTupleAdapter<Stream>;

    static const Int BitsPerSymbol 	= Container::Types::BitsPerSymbol;
    static const Int Symbols 		= Container::Types::Symbols;


    Int symbol() const
    {
        auto& self  = this->self();
        return std::get<0>(self.ctr().template read_leaf_entry<IntList<0>>(self.leaf(), self.idx()));
    }


    void setSymbol(Int symbol)
    {
    	auto& self  = this->self();

    	self.ctr().template update_stream_entry<0, IntList<0>>(self, std::make_tuple(symbol));
    }


    void insert_symbol(Int symbol)
    {
        MEMORIA_ASSERT(symbol, <, Symbols);

    	auto& self  = this->self();
        auto& ctr   = self.ctr();

    	ctr.insert_entry(
    			self,
    			InputTupleAdapter<0>::convert(symbol)
    	);

    	self.skipFw(1);
    }


    template <typename T>
    struct ReadWalker {
    	T& data_;
    	CtrSizeT processed_ = 0;
    	const CtrSizeT max_;

    	ReadWalker(T& data): data_(data), max_(data.size()) {}

    	template <typename NodeTypes>
    	Int treeNode(const LeafNode<NodeTypes>* leaf, Int start)
    	{
    		return std::get<0>(leaf->template processSubstreams<IntList<0, 0>>(*this, start));
    	}

    	template <typename StreamObj>
    	Int stream(const StreamObj* obj, Int start)
    	{
    		if (obj != nullptr)
    		{
    			Int size 		= obj->size();
    			Int remainder 	= size - start;

    			Int to_read = (processed_ + remainder < max_) ? remainder : (max_ - processed_);

    			obj->read(&data_, start, processed_, to_read);

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



    BigInt read(SymbolsBuffer<BitsPerSymbol>& data)
    {
        auto& self = this->self();
        ReadWalker<SymbolsBuffer<BitsPerSymbol>> target(data);

        return self.ctr().readStream2(self, target);
    }


    void insert_symbols(SymbolsBuffer<BitsPerSymbol>& data)
    {
    	auto& self = this->self();
    	auto& model = self.ctr();

    	auto& leaf = self.leaf();

    	seq_dense::SymbolsInputBufferProvider<BitsPerSymbol> provider(data);

    	auto result = model.insertBuffers(leaf, self.idx(), provider);

    	self.leaf() = result.leaf();
    	self.idx() = result.position();

    	self.refresh();

    	model.markCtrUpdated();
    }

    void check(const char* source = nullptr) const
    {
    	auto& self = this->self();

    	auto tmp = self;

    	tmp.refresh();

    	if (self.cache() != tmp.cache())
    	{
    		throw TestException(
    				source != nullptr ? source : MA_SRC,
    				SBuf()<<"Iterator cache mismatch: having: "<<self.cache()<<", should be: "<<tmp.cache()
    		);
    	}
    }


MEMORIA_ITERATOR_PART_END


#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::seq_dense::IterMiscName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




#undef M_TYPE
#undef M_PARAMS


}



#endif
