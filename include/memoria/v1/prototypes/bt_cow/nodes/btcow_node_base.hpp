
// Copyright 2017 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/core/types/typehash.hpp>
#include <memoria/v1/core/types/algo/select.hpp>
#include <memoria/v1/core/tools/reflection.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/packed/tools/packed_allocator.hpp>
#include <memoria/v1/core/packed/tools/packed_dispatcher.hpp>
#include <memoria/v1/core/packed/tools/packed_tools.hpp>

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/prototypes/bt/bt_names.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools_packed_struct_list_builder.hpp>

#include <memoria/v1/prototypes/bt/tools/bt_tools_size_list_builder.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools_substreamgroup_dispatcher.hpp>
#include <memoria/v1/core/tools/uuid.hpp>


namespace memoria {
namespace v1 {
namespace btcow {

template <typename Metadata, typename Base_>
class TreeNodeBase: public Base_ {
public:
    static const UInt VERSION = 1;
    typedef Base_                               Base;

    typedef typename Base::ID                   ID;


//    static_assert(std::is_trivial<Metadata>::value, "TreeNodeBase: metadata must be a trivial type");
//    static_assert(std::is_trivial<Base_>::value,    "TreeNodeBase: base must be a trivial type");
//    static_assert(std::is_trivial<ID>::value,       "TreeNodeBase: ID must be a trivial type");

    static const Int StreamsStart               = 1;

private:

    Int root_;
    Int leaf_;
    Int level_;

    ID  next_leaf_id_;

    PackedAllocator allocator_;

public:

    enum {METADATA = 0};

    using MyType = TreeNodeBase<Metadata, Base>;

    TreeNodeBase() = default;

    inline bool is_root() const {
        return root_;
    }

    void set_root(bool root) {
        root_ = root;
    }

    inline bool is_leaf() const {
        return leaf_;
    }

    void set_leaf(bool leaf) {
        leaf_ = leaf;
    }

    const Int& level() const
    {
        return level_;
    }

    Int& level()
    {
        return level_;
    }

    const ID& next_leaf_id() const {
        return next_leaf_id_;
    }

    ID& next_leaf_id() {
        return next_leaf_id_;
    }

    PackedAllocator* allocator() {
        return &allocator_;
    }

    const PackedAllocator* allocator() const {
        return &allocator_;
    }

    bool has_root_metadata() const
    {
        return allocator()->element_size(METADATA) >= (int)sizeof(Metadata);
    }

    const Metadata& root_metadata() const
    {
        return *allocator()->template get<Metadata>(METADATA);
    }

    Metadata& root_metadata()
    {
        MEMORIA_V1_ASSERT_TRUE(!allocator_.is_empty(METADATA));
        return *allocator()->template get<Metadata>(METADATA);
    }

    void setMetadata(const Metadata& meta)
    {
        if (!has_root_metadata())
        {
            allocator_.template allocate<Metadata>(METADATA);
        }

        root_metadata() = meta;
    }

    void clearMetadata() {
        allocator_.free(METADATA);
    }

    bool canConvertToRoot() const
    {
        if (!has_root_metadata())
        {
            const Int metadata_size = PackedAllocator::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
            return allocator_.free_space() >= metadata_size;
        }
        else {
            return true;
        }
    }



    bool shouldBeMergedWithSiblings() const
    {
        Int client_area = allocator_.client_area();
        Int used        = allocator_.allocated();

        return used < client_area / 2;
    }

public:

    void initAllocator(Int entries)
    {
        Int page_size = this->page_size();
        MEMORIA_V1_ASSERT(page_size, >, (int)sizeof(MyType) + PackedAllocator::my_size());

        allocator_.setTopLevelAllocator();
        allocator_.init(page_size - sizeof(MyType) + PackedAllocator::my_size(), entries);
    }

    void transferDataTo(MyType* other) const
    {
        for (Int c = 0; c < StreamsStart; c++)
        {
            other->allocator_.importBlock(c, &allocator_, c);
        }
    }

    void resizePage(Int new_size)
    {
        this->page_size() = new_size;
        allocator_.resizeBlock(new_size - sizeof(MyType) + PackedAllocator::my_size());
    }

public:

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);

        handler->value("ROOT",  &root_);
        handler->value("LEAF",  &leaf_);
        handler->value("LEVEL", &level_);

        handler->value("NEXT_LEAF_ID_", &next_leaf_id_);

        allocator()->generateDataEvents(handler);

        if (has_root_metadata())
        {
            const Metadata& meta = this->root_metadata();
            meta.generateDataEvents(handler);
        }
    }


    template <template <typename> class FieldFactory>
    void serialize(SerializationData& buf) const
    {
        Base::template serialize<FieldFactory>(buf);

        FieldFactory<Int>::serialize(buf, root_);
        FieldFactory<Int>::serialize(buf, leaf_);
        FieldFactory<Int>::serialize(buf, level_);

        FieldFactory<ID>::serialize(buf, next_leaf_id_);

        allocator()->serialize(buf);

        if (has_root_metadata())
        {
            const Metadata& meta = this->root_metadata();
            FieldFactory<Metadata>::serialize(buf, meta);
        }
    }


    template <template <typename> class FieldFactory>
    void deserialize(DeserializationData& buf)
    {
        Base::template deserialize<FieldFactory>(buf);

        FieldFactory<Int>::deserialize(buf, root_);
        FieldFactory<Int>::deserialize(buf, leaf_);
        FieldFactory<Int>::deserialize(buf, level_);

        FieldFactory<ID>::deserialize(buf, next_leaf_id_);

        allocator()->deserialize(buf);

        if (has_root_metadata())
        {
            Metadata& meta = this->root_metadata();
            FieldFactory<Metadata>::deserialize(buf, meta);
        }
    }

    void copyFrom(const MyType* page)
    {
        Base::copyFrom(page);

        this->set_root(page->is_root());
        this->set_leaf(page->is_leaf());

        this->level()       = page->level();

        this->next_leaf_id() = page->next_leaf_id();

        //FIXME: copy allocator?
        //FIXME: copy root metadata ?
    }
};

}

template <typename Metadata, typename Base>
struct TypeHash<btcow::TreeNodeBase<Metadata, Base>> {
    typedef btcow::TreeNodeBase<Metadata, Base> TargetType;

    static const UInt Value = HashHelper<
            TypeHash<Base>::Value,
            TargetType::VERSION,
            TypeHash<Int>::Value,
            TypeHash<Int>::Value,
            TypeHash<Int>::Value,
            TypeHash<typename TargetType::ID>::Value,
            TypeHash<Int>::Value,
            TypeHash<Metadata>::Value
    >::Value;
};





}}
