
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINER_VECTORMAP_ITERATOR_CRUD_HPP
#define _MEMORIA_CONTAINER_VECTORMAP_ITERATOR_CRUD_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/idata.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/containers/vector_map/vmap_names.hpp>
#include <memoria/containers/vector_map/vmap_tools.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {




MEMORIA_ITERATOR_PART_NO_CTOR_BEGIN(memoria::vmap::ItrCRUDName)

    typedef Ctr<typename Types::CtrTypes>                       				Container;


    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBase                                             NodeBase;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Container::Value                                           Value;
    typedef typename Container::Key                                             Key;
    typedef typename Container::Accumulator                                     Accumulator;

    typedef typename Container::DataSource                                      DataSource;
    typedef typename Container::DataTarget                                      DataTarget;
    typedef typename Container::LeafDispatcher                                  LeafDispatcher;
    typedef typename Container::Position                                        Position;

    typedef typename Container::Types::CtrSizeT                                 CtrSizeT;

    CtrSizeT read(DataTarget& tgt)
    {
        auto& self = this->self();

        vmap::VectorMapTarget target(&tgt);

        Position pos;

        pos[1] = self.idx();

        return self.ctr().readStreams(self, pos, target)[1];
    }

    vector<Value>
    read()
    {
        auto& self = this->self();
        vector<Value> data(self.blob_size());

        MemBuffer<Value> buf(data);

        self.read(buf);

        return data;
    }


    void insert(DataSource& src)
    {
        auto& self = this->self();

        MEMORIA_ASSERT_TRUE(self.stream() == 1);

        self.ctr().insertData(self, src);
    }

    void insert(const std::vector<Value>& data)
    {
        auto& self = this->self();

        MEMORIA_ASSERT_TRUE(self.stream() == 1);

        MemBuffer<const Value> src(data);

        self.ctr().insertData(self, src);
    }

    void remove(CtrSizeT size)
    {
        auto& self = this->self();

        MEMORIA_ASSERT_TRUE(self.stream() == 1);

        self.ctr().removeData(self, size);
    }



    struct ReadValueFn {
        typedef Value ReturnType;
        typedef Value ResultType;

        template <typename Node>
        ReturnType treeNode(const Node* node, Int offset)
        {
            return node->template processStreamRtn<1>(*this, offset);
        }

        template <Int StreamIdx, typename StreamType>
        ResultType stream(const StreamType* obj, Int offset)
        {
            return obj->value(offset);
        }
    };

    Value value()
    {
        auto& self = this->self();
        MEMORIA_ASSERT_TRUE(self.stream() == 1);

        return LeafDispatcher::dispatchConstRtn(self.leaf(), ReadValueFn(), self.idx());
    }

    struct UpdateFn {

        template <typename Node>
        void treeNode(Node* node, Int offset, const Accumulator& keys)
        {
            node->template processStream<0>(*this, offset, keys);
        }

        template <Int StreamIdx, typename StreamType>
        void stream(StreamType* obj, Int offset, const Accumulator& keys)
        {
            obj->updateUp(0, offset, std::get<0>(keys)[0]);
            obj->updateUp(1, offset, std::get<0>(keys)[1]);
            obj->reindex();
        }
    };

    void update(const Accumulator& accum)
    {
        auto& self = this->self();

        MEMORIA_ASSERT_TRUE(self.stream() == 0);

        NodeBaseG& leaf = self.leaf();

        self.ctr().updatePageG(leaf);
        LeafDispatcher::dispatch(leaf, UpdateFn(), self.idx(), accum);

        self.ctr().updateParent(leaf, accum);

        self.cache().addToEntry(
            std::get<0>(accum)[0],
            std::get<0>(accum)[1]
        );
    }



MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::vmap::ItrApiName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




}

#undef M_TYPE
#undef M_PARAMS

#endif
