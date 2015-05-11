
// Copyright Victor Smirnov 2013-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_vctr_TOOLS_HPP
#define _MEMORIA_CONTAINERS_vctr_TOOLS_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>

#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/idata.hpp>
#include <memoria/core/container/container.hpp>

#include <memoria/prototypes/bt/layouts/bt_input_buffer.hpp>

namespace memoria       {
namespace mvector       {



template <typename T>
class ArrayInputBufferProvider: public bt::InputBufferProvider<Int, T> {

	using Position = Int;

	std::vector<T>& data_;
	Position start_;
	Position size_;

	bool next_ = true;

public:


	ArrayInputBufferProvider(std::vector<T>& data, Position start = 0): data_(data), start_(0), size_(data.size()) {}
	ArrayInputBufferProvider(std::vector<T>& data, Position start, Position size): data_(data), start_(0), size_(size)  {}

	virtual Position start() const {
		return start_;
	}

	virtual Position size()	const {
		return size_;
	}

	virtual Position zero()	const {return 0;}

	virtual const T* buffer() const {
		return &data_[0];
	}

	virtual void consumed(Position sizes) {
		start_ += sizes;
	}

	virtual bool isConsumed() {
		return start_ >= size_;
	}

	virtual void nextBuffer() {
		next_ = false;
	}

	virtual bool hasData() const {
		return next_ || start_ < size_;
	}
};


class VectorSource: public ISource {

    IDataBase* source_;
public:
    VectorSource(IDataBase* source): source_(source) {}

    virtual Int streams()
    {
        return 1;
    }

    virtual IData* stream(Int stream)
    {
        return source_;
    }

    virtual void newNode(INodeLayoutManager* layout_manager, BigInt* sizes)
    {
        Int allocated[1] = {0};
        Int capacity = layout_manager->getNodeCapacity(allocated, 0);

        sizes[0] = capacity;
    }

    virtual BigInt getTotalNodes(INodeLayoutManager* manager)
    {
        Int sizes[1] = {0};

        SizeT capacity  = manager->getNodeCapacity(sizes, 0);
        SizeT remainder = source_->getRemainder();

        return remainder / capacity + (remainder % capacity ? 1 : 0);
    }
};


class VectorTarget: public ITarget {

    IDataBase* target_;
public:
    VectorTarget(IDataBase* target): target_(target) {}

    virtual Int streams()
    {
        return 1;
    }

    virtual IData* stream(Int stream)
    {
        return target_;
    }
};




template <typename Iterator, typename Container>
class VectorIteratorPrefixCache: public bt::BTreeIteratorCache<Iterator, Container> {
    typedef bt::BTreeIteratorCache<Iterator, Container>                 Base;
    typedef typename Container::Position                                        Position;
    typedef typename Container::Accumulator                                     Accumulator;

    Position prefix_;
    Position current_;

public:

    VectorIteratorPrefixCache(): Base(), prefix_(), current_() {}

    const BigInt& prefix(int num = 0) const
    {
        return prefix_[num];
    }

    const Position& sizePrefix() const
    {
        return prefix_;
    }

    void setSizePrefix(const Position& prefix)
    {
        prefix_ = prefix;
    }

    const Position& prefixes() const
    {
        return prefix_;
    }

    void nextKey(bool end)
    {
        prefix_ += current_;

        Clear(current_);
    };

    void prevKey(bool start)
    {
        prefix_ -= current_;

        Clear(current_);
    };

    void Prepare()
    {
        if (Base::iterator().key_idx() >= 0)
        {
//            current_ = Base::iterator().getRawKeys();
        }
        else {
            Clear(current_);
        }
    }

    void setup(const Position& prefix)
    {
        prefix_ = prefix;
    }

    void setup(BigInt prefix)
    {
        prefix_[0] = prefix;
    }

    void Clear(Position& v) {v = Position();}

    void initState()
    {
        Clear(prefix_);

        auto node = Base::iterator().leaf();

        auto& ctr = Base::iterator().ctr();

        while (!node->is_root())
        {
            Int idx = node->parent_idx();
            node = ctr.getNodeParent(node);

            Accumulator acc;

            ctr.sums(node, 0, idx, acc);

            prefix_ += std::get<0>(acc)[0];
        }
    }

private:

    void init_()
    {

    }

};



template <
    typename I, typename C
>
std::ostream& operator<<(std::ostream& out, const VectorIteratorPrefixCache<I, C>& cache)
{
    out<<"VectorIteratorPrefixCache[";
    out<<"prefixes: "<<cache.prefixes();
    out<<"]";

    return out;
}





}
}

#endif
