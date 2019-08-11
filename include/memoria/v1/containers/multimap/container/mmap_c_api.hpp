
// Copyright 2015 Victor Smirnov
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


#include <memoria/v1/containers/multimap/mmap_names.hpp>
#include <memoria/v1/containers/multimap/mmap_tools.hpp>
#include <memoria/v1/containers/multimap/mmap_output_entries.hpp>
#include <memoria/v1/containers/multimap/mmap_output_values.hpp>
#include <memoria/v1/containers/multimap/mmap_output_keys.hpp>
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <memoria/v1/api/multimap/multimap_input.hpp>
#include <memoria/v1/api/multimap/multimap_entry.hpp>
#include <memoria/v1/core/tools/static_array.hpp>

#include <vector>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(mmap::CtrApiName)
public:
    using Types = typename Base::Types;
    using typename Base::IteratorPtr;

protected:
    using typename Base::Profile;
    using typename Base::NodeBaseG;
    using typename Base::NodeDispatcher;
    using typename Base::LeafDispatcher;
    using typename Base::BranchDispatcher;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::BlockUpdateMgr;
    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;

    using Key   = typename Types::Key;
    using Value = typename Types::Value;

    using KeyView   = typename DataTypeTraits<Key>::ViewType;
    using ValueView = typename DataTypeTraits<Value>::ViewType;

    using CtrApiTypes = ICtrApiTypes<typename Types::ContainerTypeName, Profile>;

public:
    IteratorPtr begin() {
        return self().template seek_stream<0>(0);
    }

    IteratorPtr end() {
        auto& self = this->self();
        auto ii = self.template seek_stream<1>(self.sizes()[1]);

        ii->stream() = 1;
        ii->toStructureStream();

        return ii;
    }

    IteratorPtr seek(CtrSizeT idx)
    {
        auto& self = this->self();
        auto ii = self.template seek_stream<0>(idx);

        ii->stream() = 0;

        ii->toStructureStream();

        return ii;
    }

    CtrSharedPtr<IEntriesIterator<CtrApiTypes, Profile>> seek_entry(CtrSizeT idx)
    {
        auto& self = this->self();
        auto ii = self.template seek_stream<0>(idx);

        ii->stream() = 0;

        ii->toStructureStream();

        auto ptr = ctr_make_shared<mmap::EntriesIteratorImpl<CtrApiTypes, Profile, IteratorPtr>>(ii);

        return memoria_static_pointer_cast<IEntriesIterator<CtrApiTypes, Profile>>(ptr);
    }

    CtrSharedPtr<IKeysIterator<CtrApiTypes, Profile>> keys()
    {
        auto& self = this->self();
        auto ii = self.template seek_stream<0>(0);

        ii->stream() = 0;
        ii->toStructureStream();

        auto ptr = ctr_make_shared<mmap::KeysIteratorImpl<CtrApiTypes, Profile, IteratorPtr>>(ii);

        return memoria_static_pointer_cast<IKeysIterator<CtrApiTypes, Profile>>(ptr);
    }

    CtrSharedPtr<IValuesIterator<CtrApiTypes, Profile>> find_entry(KeyView key)
    {
        auto& self = this->self();
        auto ii = self.find(key);

        if (ii->is_found(key))
        {
            ii->to_values();
            auto ptr = ctr_make_shared<mmap::ValuesIteratorImpl<CtrApiTypes, Profile, IteratorPtr>>(ii);
            return memoria_static_pointer_cast<IValuesIterator<CtrApiTypes, Profile>>(ptr);
        }
        else {
            return CtrSharedPtr<IValuesIterator<CtrApiTypes, Profile>>{};
        }
    }

    CtrSizeT size() const {
        return self().sizes()[0];
    }

    IteratorPtr find(KeyView key)
    {
        return self().template find_max_ge<IntList<0, 1>>(0, key);
    }

    IteratorPtr find_or_create(KeyView key)
    {
        auto& self = this->self();

        auto iter = self.find(key);

        if (!iter->is_found(key))
        {
            iter->insert_key(key);
        }

        return iter;
    }


    void append_entries(io::IOVectorProducer& producer)
    {
        self().end()->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
    }

//    void append_entry(KeyView key, absl::Span<const ValueView> values)
//    {
//        io::MultimapEntryIOVector<CtrApiTypes> iovector(&key, values.data(), values.size());
//        self().end()->insert_iovector(iovector, 0, std::numeric_limits<int64_t>::max());
//    }



    void prepend_entries(io::IOVectorProducer& producer)
    {
        self().begin()->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
    }

//    void prepend_entry(KeyView key, absl::Span<const ValueView> values)
//    {
//        io::MultimapEntryIOVector<CtrApiTypes> iovector(&key, values.data(), values.size());
//        self().begin()->insert_iovector(iovector, 0, std::numeric_limits<int64_t>::max());
//    }



    void insert_entries(KeyView before, io::IOVectorProducer& producer)
    {
        auto ii = self().find(before);
        ii->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
    }

//    void insert_entry(KeyView before, KeyView key, absl::Span<const ValueView> values)
//    {
//        io::MultimapEntryIOVector<CtrApiTypes> iovector(&key, values.data(), values.size());

//        auto ii = self().find(before);
//        ii->insert_iovector(iovector, 0, std::numeric_limits<int64_t>::max());
//    }


    bool upsert(KeyView key, io::IOVectorProducer& producer)
    {
        auto ii = self().find(key);

        if (ii->is_found(key))
        {
            ii->remove(1);
            ii->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());

            return true;
        }

        ii->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());

        return false;
    }


//    bool upsert(KeyView key, absl::Span<const ValueView> values)
//    {
//        auto ii = self().find(key);

//        io::MultimapEntryIOVector<CtrApiTypes> iovector(&key, values.data(), values.size());

//        if (ii->is_found(key))
//        {
//            ii->remove(1);
//            ii->insert_iovector(iovector, 0, std::numeric_limits<int64_t>::max());

//            return true;
//        }

//        ii->insert_iovector(iovector, 0, std::numeric_limits<int64_t>::max());

//        return false;
//    }


    bool contains(const KeyView& key)
    {
        auto iter = self().find(key);
        return iter->is_found(key);
    }

    bool remove(const KeyView& key)
    {
        auto ii = self().find(key);

        if (ii->is_found(key)) {
            ii->remove(1);
            return true;
        }

        return false;
    }


    bool remove_all(const KeyView& from, const KeyView& to)
    {
        auto ii = self().find(from);
        auto jj = self().find(to);

        return ii->remove_all(*jj.get());
    }

    bool remove_from(const KeyView& from)
    {
        auto ii = self().find(from);
        auto jj = self().end();

        return ii->remove_all(*jj.get());
    }


    bool remove_before(const KeyView& to)
    {
        auto ii = self().begin();
        auto jj = self().find(to);

        return ii->remove_all(*jj.get());
    }


protected:

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(mmap::CtrApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}}
