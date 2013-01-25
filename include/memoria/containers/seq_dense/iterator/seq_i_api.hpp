
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQDENSE_ITERATOR_API_HPP
#define _MEMORIA_CONTAINERS_SEQDENSE_ITERATOR_API_HPP

#include <memoria/containers/seq_dense/names.hpp>
#include <memoria/prototypes/sequence/tools.hpp>
#include <memoria/core/container/iterator.hpp>

#include <memoria/core/tools/walkers.hpp>
#include <memoria/core/tools/sum_walker.hpp>

#include <memoria/core/tools/symbol_sequence.hpp>

#include <iostream>

namespace memoria    {

using namespace memoria::btree;


MEMORIA_ITERATOR_PART_BEGIN(memoria::seq_dense::IterAPIName)

    typedef typename Base::NodeBase                                             NodeBase;
    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::Container                                            Container;

    typedef typename Container::Key                                             Key;

    typedef typename Container::Page                                      		PageType;
    typedef typename Container::ID                                        		ID;

    typedef typename Types::Allocator                                			Allocator;
    typedef typename Types::DataPage                                 			DataPage;
    typedef typename Types::DataPageG                                			DataPageG;
    typedef typename Types::Pages::NodeDispatcher                    			NodeDispatcher;
    typedef typename Types::ElementType                              			ElementType;

    typedef typename Types::IDataSourceType                                 	IDataSourceType;

    typedef typename Base::TreePath                                             TreePath;

    static const Int Indexes = Container::Indexes;

    typedef SymbolSequence<
    		Types::BitsPerSymbol,
    		ElementType,
    		typename DataPage::IndexType
    > 																			SymbolSequenceType;

    
    MEMORIA_PUBLIC SymbolSequenceType subVector(BigInt length) const
    {
        MyType tmp = *me();
        return tmp.readSequence(length);
    }

    MEMORIA_PUBLIC SymbolSequenceType readSequence(BigInt length)
    {
        BigInt max_size = me()->model().size() - me()->pos();

        if (length > max_size)
        {
            length = max_size;
        }

        SymbolSequenceType seq(length);
        seq.resize(length);

        auto tgt = seq.target();

        me()->read(tgt);

        return seq;
    }

MEMORIA_ITERATOR_PART_END


#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::seq_dense::IterAPIName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS


}



#endif
