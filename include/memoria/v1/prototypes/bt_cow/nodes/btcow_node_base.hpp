
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

#include <memoria/v1/core/types.hpp>
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
    static const uint32_t VERSION = 1;
    typedef Base_                               Base;

    typedef typename Base::ID                   ID;


//    static_assert(std::is_trivial<Metadata>::value, "TreeNodeBase: metadata must be a trivial type");
//    static_assert(std::is_trivial<Base_>::value,    "TreeNodeBase: base must be a trivial type");
//    static_assert(std::is_trivial<ID>::value,       "TreeNodeBase: ID must be a trivial type");

    static const int32_t StreamsStart               = 1;

private:

    int32_t root_;
    int32_t leaf_;
    int32_t level_;

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

    const int32_t& level() const
    {
        return level_;
    }

    int32_t& level()
    {
        return level_;
    }

    const ID& next_leaf_id() const {
        return next_leaf_id_;
    }

    ID& next_leaf_id() {
        return next_leaf_id_;
    }

    PackedAllocator* store() {
        return &allocator_;
    }

    const PackedAllocator* store() const {
        return &allocator_;
    }

    bool has_root_metadata() const
    {
        return store()->element_size(METADATA) >= (int)sizeof(Metadata);
    }

    const Metadata& root_metadata() const
    {
        return *store()->template get<Metadata>(METADATA);
    }

    Metadata& root_metadata()
    {
        MEMORIA_V1_ASSERT_TRUE(!allocator_.is_empty(METADATA));
        return *store()->template get<Metadata>(METADATA);
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
            const int32_t metadata_size = PackedAllocator::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
            return allocator_.free_space() >= metadata_size;
        }
        else {
            return true;
        }
    }



    bool shouldBeMergedWithSiblings() const
    {
        int32_t client_area = allocator_.client_area();
        int32_t used        = allocator_.allocated();

        return used < client_area / 2;
    }

public:

    void initAllocator(int32_t entries)
    {
        int32_t block_size = this->memory_block_size();
        MEMORIA_V1_ASSERT(block_size, >, (int)sizeof(MyType) + PackedAllocator::my_size());

        allocator_.setTopLevelAllocator();
        allocator_.init(block_size - sizeof(MyType) + PackedAllocator::my_size(), entries);
    }

    void transferDataTo(MyType* other) const
    {
        for (int32_t c = 0; c < StreamsStart; c++)
        {
            other->allocator_.importBlock(c, &allocator_, c);
        }
    }

    void resizePage(int32_t new_size)
    {
        this->memory_block_size() = new_size;
        allocator_.resizeBlock(new_size - sizeof(MyType) + PackedAllocator::my_size());
    }

public:

    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);

        handler->value("ROOT",  &root_);
        handler->value("LEAF",  &leaf_);
        handler->value("LEVEL", &level_);

        handler->value("NEXT_LEAF_ID_", &next_leaf_id_);

        store()->generateDataEvents(handler);

        if (has_root_metadata())
        {
            const Metadata& meta = this->root_metadata();
            meta.generateDataEvents(handler);
        }
    }


    template <typename SerializationData>
    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);

        FieldFactory<int32_t>::serialize(buf, root_);
        FieldFactory<int32_t>::serialize(buf, leaf_);
        FieldFactory<int32_t>::serialize(buf, level_);

        FieldFactory<ID>::serialize(buf, next_leaf_id_);

        store()->serialize(buf);

        if (has_root_metadata())
        {
            const Metadata& meta = this->root_metadata();
            FieldFactory<Metadata>::serialize(buf, meta);
        }
    }


    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        FieldFactory<int32_t>::deserialize(buf, root_);
        FieldFactory<int32_t>::deserialize(buf, leaf_);
        FieldFactory<int32_t>::deserialize(buf, level_);

        FieldFactory<ID>::deserialize(buf, next_leaf_id_);

        store()->deserialize(buf);

        if (has_root_metadata())
        {
            Metadata& meta = this->root_metadata();
            FieldFactory<Metadata>::deserialize(buf, meta);
        }
    }

    void copyFrom(const MyType* block)
    {
        Base::copyFrom(block);

        this->set_root(block->is_root());
        this->set_leaf(block->is_leaf());

        this->level()       = block->level();

        this->next_leaf_id() = block->next_leaf_id();

        //FIXME: copy allocator?
        //FIXME: copy root metadata ?
    }
};

}

template <typename Metadata, typename Base>
struct TypeHash<btcow::TreeNodeBase<Metadata, Base>> {
    typedef btcow::TreeNodeBase<Metadata, Base> TargetType;

    static const uint32_t Value = HashHelper<
            TypeHash<Base>::Value,
            TargetType::VERSION,
            TypeHash<int32_t>::Value,
            TypeHash<int32_t>::Value,
            TypeHash<int32_t>::Value,
            TypeHash<typename TargetType::ID>::Value,
            TypeHash<int32_t>::Value,
            TypeHash<Metadata>::Value
    >::Value;
};





}}
