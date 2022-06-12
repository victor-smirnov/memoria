
// Copyright 2014 Victor Smirnov
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


#include <memoria/containers/map/map_names.hpp>
#include <memoria/containers/map/map_tools.hpp>
#include <memoria/containers/map/map_chunk_impl.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/core/tools/optional.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(map::CtrRApiName)

    using Types = typename Base::Types;

    using typename Base::TreeNodePtr;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::Profile;
    using typename Base::ApiProfileT;
    using typename Base::CtrSizeT;
    using typename Base::ShuttleTypes;

    using Key   = typename Types::Key;
    using Value = typename Types::Value;

    using KeyView   = typename DataTypeTraits<Key>::ViewType;
    using ValueView = typename DataTypeTraits<Value>::ViewType;

    using MapChunkT = MapChunk<Key, Value, ApiProfile<Profile>>;
    using ChunkSharedPtr = IterSharedPtr<MapChunkT>;

    struct MapChunkTypes: Types {
      using KeyType = Key;
      using ValueType = Value;
      using ShuttleTypes = typename Base::ShuttleTypes;
    };

    using ChunkImplT = MapChunkImpl<MapChunkTypes>;


    template <typename LeafPath>
    using TargetType = typename Types::template TargetType<LeafPath>;

    using KeysPath = IntList<0, 1>;
    using ValuesPath = IntList<0, 2>;

    template <typename ShuttleTypes>
    using FindShuttle = bt::FindForwardShuttle<ShuttleTypes, KeysPath, ChunkImplT>;

    using typename Base::BranchNodeExtData;
    using typename Base::LeafNodeExtData;
    using typename Base::ContainerTypeName;

    void configure_types(
        const ContainerTypeName& type_name,
        BranchNodeExtData& branch_node_ext_data,
        LeafNodeExtData& leaf_node_ext_data
    ) {

    }

    virtual ChunkSharedPtr seek_entry(CtrSizeT num) const
    {
        auto& self = this->self();
        return self.ctr_seek_entry(num);
    }

    IterSharedPtr<ChunkImplT> ctr_seek_entry(CtrSizeT num) const
    {
        auto& self = this->self();
        return self.ctr_descend(
                    TypeTag<ChunkImplT>{},
                    TypeTag<bt::SkipForwardShuttle<ShuttleTypes, 0, ChunkImplT>>{},
                    num
        );
    }

    IterSharedPtr<ChunkImplT> ctr_map_find(const KeyView& k) const
    {
        return self().ctr_descend(
            TypeTag<ChunkImplT>{},
            bt::ShuttleTag<FindShuttle>{},
            k, 0, SearchType::GE
        );
    }


    virtual IterSharedPtr<MapChunk<Key, Value, ApiProfileT>> find(KeyView key) const
    {
        return self().ctr_map_find(key);
    }



//    bool upsert_key(KeyView key, ValueView value)
//    {
//      auto& self = this->self();
//      auto iter = self.ctr_map_find(key);

//      if (iter->is_found(key))
//      {
//        self.ctr_update_map_entry(std::move(iter), value);
//        return true;
//      }
//      else {
//        self.ctr_insert_map_entry(std::move(iter), key, value);
//        return false;
//      }
//    }



//    bool remove_key(KeyView key)
//    {
//      auto& self = this->self();
//      auto iter = self.ctr_map_find(key);

//      if (iter->is_found(key))
//      {
//        self.ctr_remove_map_entry(std::move(iter));
//        return true;
//      }

//      return false;
//    }


//    ChunkSharedPtr append(io::IOVectorProducer& producer)
//    {
//        auto& self = this->self();

//        auto iter = self.ctr_seek_entry(self.size());

//        auto jj = self.ctr_insert_iovector2(std::move(iter), producer, 0, std::numeric_limits<CtrSizeT>::max());
//        return memoria_static_pointer_cast<ChunkImplT>(jj);
//    }

//    ChunkSharedPtr prepend(io::IOVectorProducer& producer)
//    {
//        auto& self = this->self();
//        auto iter = self.ctr_seek_entry(0);
//        auto jj = self.ctr_insert_iovector2(std::move(iter), producer, 0, std::numeric_limits<CtrSizeT>::max());
//        return memoria_static_pointer_cast<ChunkImplT>(jj);
//    }

//    ChunkSharedPtr insert(KeyView before, io::IOVectorProducer& producer)
//    {
//        auto& self = this->self();

//        auto iter = self.ctr_map_find(before);

//        if (iter->is_found(before))
//        {
//            MEMORIA_MAKE_GENERIC_ERROR("Requested key is found. Can't insert enties this way.").do_throw();
//        }
//        else {
//            auto jj = self.ctr_insert_iovector2(std::move(iter), producer, 0, std::numeric_limits<CtrSizeT>::max());
//            return memoria_static_pointer_cast<ChunkImplT>(jj);
//        }
//    }






//    Optional<Datum<Value>> remove_and_return(KeyView key)
//    {
//        auto& self = this->self();

//        auto iter = self.ctr_map_find(key);

//        if (iter->is_found(key))
//        {
//            auto val = iter->current_value();
//            self.ctr_remove_map_entry(std::move(iter));
//            return Datum<Value>(val);
//        }
//        else {
//            return Optional<Datum<Value>>{};
//        }
//    }

//    Optional<Datum<Value>> replace_and_return(KeyView key, ValueView value)
//    {
//      auto& self = this->self();

//      auto iter = self.ctr_map_find(key);

//      if (iter->is_found(key))
//      {
//        auto prev = iter->current_value();
//        self.ctr_update_map_entry(std::move(iter), value);
//        return Datum<Value>(prev);
//      }
//      else {
//        self.ctr_insert_map_entry(std::move(iter), key, value);
//        return Optional<Datum<Value>>{};
//      }
//    }


//    virtual void with_value(
//            KeyView key,
//            std::function<Optional<Datum<Value>> (Optional<Datum<Value>>)> value_fn
//    )
//    {
//      using OptionalValue = Optional<Datum<Value>>;
//      auto& self = this->self();

//      auto iter = self.ctr_map_find(key);
//      if (iter->is_found(key))
//      {
//        auto new_value = value_fn(Datum<Value>(iter->current_value()));
//        if (new_value) {
//          self.ctr_update_map_entry(std::move(iter), new_value.get());
//        }
//        else {
//          self.ctr_remove_map_entry(std::move(iter));
//        }
//      }
//      else {
//        auto value = value_fn(OptionalValue{});
//        if (value) {
//          self.ctr_insert_map_entry(std::move(iter), key, value.get());
//        }
//      }
//    }

//    void remove(CtrSizeT from, CtrSizeT to)
//    {
//        self().ctr_remove(from, to);
//    }

//    void remove_from(CtrSizeT from) {
//        self().ctr_remove_from(from);
//    }

//    void remove_up_to(CtrSizeT pos) {
//        self().ctr_remove_up_to(pos);
//    }

//    template <typename IterT>
//    void ctr_remove_map_entry(IterT&& iter)
//    {
//      auto idx = iter->iter_leaf_position();
//      self().ctr_remove_entry(iter->path(), idx);
//    }

//    template <typename IterT>
//    void ctr_insert_map_entry(IterT&& iter, KeyView key, ValueView value) {
//      self().ctr_insert_entry(
//          std::move(iter),
//          map::KeyValueEntry<KeyView, ValueView, CtrSizeT>(key, value)
//      );
//    }

//    template <typename IterT>
//    void ctr_update_map_entry(IterT&& iter, ValueView value)
//    {
//      self().template ctr_update_entry2<ValuesPath>(std::move(iter), map::ValueBuffer<ValueView>(value));
//    }


MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(map::CtrRApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}
