
// Copyright 2022 Victor Smirnov
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

#include <memoria/core/arena/map.hpp>
#include <memoria/core/arena/relative_ptr.hpp>

#include <memoria/core/hermes/value.hpp>
#include <memoria/core/hermes/traits.hpp>
#include <memoria/core/hermes/datatype.hpp>
#include <memoria/core/hermes/common.hpp>
#include <memoria/core/hermes/data_object.hpp>

namespace memoria {
namespace hermes {

template <typename EntryT, typename MapT, typename Iterator>
class MapIteratorAccessor {
    ViewPtr<MapT> map_;
    Iterator iterator_;
    DocView* doc_;
    ViewPtrHolder* ptr_holder_;

public:
    using ViewType = EntryT;

    MapIteratorAccessor(const ViewPtr<MapT>& map, Iterator iterator, DocView* doc, ViewPtrHolder* ptr_holder):
        map_(map), iterator_(iterator), doc_(doc), ptr_holder_(ptr_holder)
    {}

    EntryT current() const {
        return EntryT(&iterator_, doc_, ptr_holder_);
    }

    bool operator==(const MapIteratorAccessor&other) const noexcept {
        return iterator_ == other.iterator_;
    }

    void next() {
        iterator_.next();
    }
};


template <>
class Map<Varchar, Value>: public HoldingView {
public:
    using KeyT = DataObject<Varchar>::ArenaDTContainer;

    using ArenaMap = arena::Map<KeyT*, void*>;
protected:
    mutable ArenaMap* map_;
    mutable DocView* doc_;

    friend class DocView;
    friend class Value;

    template <typename, typename>
    friend class Map;

    template <typename>
    friend class Array;

    friend class DocumentBuilder;
    friend class memoria::hermes::path::interpreter::Interpreter;
    using MapIterator = typename ArenaMap::Iterator;


    class EntryT {
        const MapIterator* iter_;
        mutable DocView* doc_;
        mutable ViewPtrHolder* ptr_holder_;

    public:
        EntryT(const MapIterator* iter, DocView* doc, ViewPtrHolder* ptr_holder):
            iter_(iter), doc_(doc), ptr_holder_(ptr_holder)
        {}

        StringValuePtr first() const {
            return StringValuePtr(StringValue(iter_->key().get(), doc_, ptr_holder_));
        }

        ValuePtr second() const
        {
            if (iter_->value().is_not_null()) {
                auto ptr = iter_->value().get();
                return ValuePtr(Value(ptr, doc_, ptr_holder_));
            }
            else {
                return ValuePtr(Value(nullptr, doc_, ptr_holder_));
            }
        }
    };

    using Accessor = MapIteratorAccessor<EntryT, Map, MapIterator>;
    using Iterator = ForwardIterator<Accessor>;

public:
    Map() noexcept : map_(), doc_() {}

    Map(void* map, DocView* doc, ViewPtrHolder* ptr_holder) noexcept :
        HoldingView(ptr_holder),
        map_(reinterpret_cast<ArenaMap*>(map)), doc_(doc)
    {}

    ViewPtr<Map, true> self() const {
        return ViewPtr<Map, true>(Map(map_, doc_, ptr_holder_));
    }

    Iterator begin() const {
        return Iterator(Accessor(self(), map_->begin(), doc_, ptr_holder_));
    }

    Iterator end() const {
        return Iterator(Accessor(self(), map_->end(), doc_, ptr_holder_));
    }

    Iterator cbegin() const {
        return Iterator(Accessor(self(), map_->begin(), doc_, ptr_holder_));
    }

    Iterator cend() const {
        return Iterator(Accessor(self(), map_->end(), doc_, ptr_holder_));
    }

    PoolSharedPtr<DocView> document() const {
        assert_not_null();
        return PoolSharedPtr<DocView>(doc_, ptr_holder_->owner(), pool::DoRef{});
    }

    ValuePtr as_value() const {
        return ValuePtr(Value(map_, doc_, ptr_holder_));
    }

    uint64_t size() const {
        assert_not_null();
        return map_->size();
    }

    bool empty() const {
        return size() == 0;
    }

    ValuePtr get(U8StringView key) const
    {
        assert_not_null();
        auto res = map_->get(key);

        if (res) {
            return ValuePtr(Value(
                res->get(), doc_, ptr_holder_
            ));
        }

        return ValuePtr();
    }


    template <typename DT>
    DataObjectPtr<DT> put_dataobject(U8StringView key, DTTViewType<DT> value);

    GenericMapPtr put_generic_map(U8StringView key);
    GenericArrayPtr put_generic_array(U8StringView key);

    ValuePtr put_hermes(U8StringView key, U8StringView str);

    void stringify(std::ostream& out,
                   DumpFormatState& state,
                   DumpState& dump_state) const
    {
        if (state.indent_size() == 0 || !is_simple_layout()) {
            do_stringify(out, state, dump_state);
        }
        else {
            DumpFormatState simple_state = state.simple();
            do_stringify(out, simple_state, dump_state);
        }
    }


    void for_each(std::function<void(U8StringView, ViewPtr<Value>)> fn) const
    {
        assert_not_null();
        map_->for_each([&](const auto& key, const auto& value){
            U8StringView kk = key->view();
            fn(kk, ViewPtr<Value>{Value(value.get(), doc_, ptr_holder_)});
        });
    }

    bool is_simple_layout() const noexcept
    {
        if (size() > 2) {
            return false;
        }

        bool simple = true;

        for_each([&](auto, auto vv){
            simple = simple && vv->is_simple_layout();
        });

        return simple;
    }

    void remove(U8StringView key);

    void* deep_copy_to(arena::ArenaAllocator& arena, DeepCopyDeduplicator& dedup) const {
        assert_not_null();
        return map_->deep_copy_to(arena, TypeHashV<Map>, doc_, ptr_holder_, dedup);
    }

    bool equals(const Map& map) const noexcept {
        return map_ == map.map_;
    }

    bool equals(GenericMapPtr map) const noexcept {
        return map_ == map->map_;
    }

protected:
    void put(StringValuePtr name, ValuePtr value);
    void put(U8StringView name, ValuePtr value);
private:
    void do_stringify(std::ostream& out, DumpFormatState state, DumpState& dump_state) const;


    void assert_not_null() const
    {
        if (MMA_UNLIKELY(map_ == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("Map<Varchar, Value> is null").do_throw();
        }
    }

    void assert_mutable();
};

namespace detail {

template <>
struct ValueCastHelper<GenericMap> {
    static GenericMapPtr cast_to(void* addr, DocView* doc, ViewPtrHolder* ref_holder) noexcept {
        return GenericMapPtr(GenericMap(
            addr,
            doc,
            ref_holder
        ));
    }
};

}


}}
