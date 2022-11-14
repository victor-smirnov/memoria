
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

#include <memoria/core/hermes/map/map_common.hpp>

namespace memoria {
namespace hermes {




template <>
class Map<Varchar, Object>: public HoldingView<Map<Varchar, Object>> {
    using Base = HoldingView<Map<Varchar, Object>>;
public:
    using KeyT = DataObject<Varchar>::ArenaDTContainer;
    using MapStorageT = arena::Map<KeyT*, void*>;

    static_assert(std::is_standard_layout_v<MapStorageT>,"");

protected:
    mutable MapStorageT* map_;
    mutable HermesCtr* doc_;
    using Base::ptr_holder_;

    friend class HermesCtr;
    friend class Object;

    template <typename, typename>
    friend class TypedGenericMap;

    template <typename, typename>
    friend class Map;

    template <typename>
    friend class Array;

    friend class HermesCtrBuilder;
    friend class memoria::hermes::path::interpreter::Interpreter;
    using MapIterator = typename MapStorageT::Iterator;


    class EntryT {
        const MapIterator* iter_;
        mutable HermesCtr* doc_;
        mutable ViewPtrHolder* ptr_holder_;

    public:
        EntryT(const MapIterator* iter, HermesCtr* doc, ViewPtrHolder* ptr_holder):
            iter_(iter), doc_(doc), ptr_holder_(ptr_holder)
        {}

        StringValuePtr first() const {
            return StringValuePtr(StringValue(iter_->key().get(), doc_, ptr_holder_));
        }

        ObjectPtr second() const
        {
            if (iter_->value().is_not_null()) {
                auto ptr = iter_->value().get();
                return ObjectPtr(Object(ptr, doc_, ptr_holder_));
            }
            else {
                return ObjectPtr(Object(nullptr, doc_, ptr_holder_));
            }
        }
    };

    using Accessor = MapIteratorAccessor<EntryT, Map, MapIterator>;

public:
    using Iterator = ForwardIterator<Accessor>;

public:
    Map() noexcept : map_(), doc_() {}

    Map(void* map, HermesCtr* doc, ViewPtrHolder* ptr_holder) noexcept :
        Base(ptr_holder),
        map_(reinterpret_cast<MapStorageT*>(map)), doc_(doc)
    {}

    bool is_null() const noexcept {
        return !map_;
    }

    bool is_not_null() const noexcept {
        return map_;
    }

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

    PoolSharedPtr<HermesCtr> document() const {
        assert_not_null();
        return PoolSharedPtr<HermesCtr>(doc_, ptr_holder_->owner(), pool::DoRef{});
    }

    ObjectPtr as_object() const {
        return ObjectPtr(Object(map_, doc_, ptr_holder_));
    }

    uint64_t size() const {
        assert_not_null();
        return map_->size();
    }

    bool empty() const {
        return size() == 0;
    }

    ObjectPtr get(U8StringView key) const
    {
        assert_not_null();
        auto res = map_->get(key);

        if (res) {
            return ObjectPtr(Object(
                res->get(), doc_, ptr_holder_
            ));
        }

        return ObjectPtr();
    }


    template <typename DT>
    DataObjectPtr<DT> put_dataobject(U8StringView key, DTTViewType<DT> value);

    ObjectMapPtr put_generic_map(U8StringView key);
    ObjectArrayPtr put_generic_array(U8StringView key);

    ObjectPtr put_hermes(U8StringView key, U8StringView str);

    void stringify(std::ostream& out,
                   DumpFormatState& state) const
    {
        if (state.cfg().spec().indent_size() == 0 || !is_simple_layout()) {
            do_stringify(out, state);
        }
        else {
            StringifyCfg cfg1 = state.cfg();
            cfg1.with_spec(StringifySpec::simple());
            DumpFormatState simple_state(cfg1);
            do_stringify(out, simple_state);
        }
    }


    void for_each(std::function<void(U8StringView, ViewPtr<Object>)> fn) const
    {
        assert_not_null();
        map_->for_each([&](const auto& key, const auto& value){
            U8StringView kk = key->view();
            fn(kk, ViewPtr<Object>{Object(value.get(), doc_, ptr_holder_)});
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
        return map_->deep_copy_to(arena, ShortTypeCode::of<Map>(), doc_, ptr_holder_, dedup);
    }

    PoolSharedPtr<GenericMap> as_generic_map() const;

    void put(StringValuePtr name, ObjectPtr value);
    void put(U8StringView name, ObjectPtr value);
private:
    void do_stringify(std::ostream& out, DumpFormatState& state) const;

    void assert_not_null() const
    {
        if (MMA_UNLIKELY(map_ == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("Map<Varchar, Object> is null").do_throw();
        }
    }

    void assert_mutable();
};


template <>
class TypedGenericMapEntry<Varchar, Object>: public GenericMapEntry {
    using MapT = Map<Varchar, Object>;
    using IteratorT = typename MapT::Iterator;

    IteratorT iter_;
    IteratorT end_;

public:
    TypedGenericMapEntry(IteratorT iter, IteratorT end):
        iter_(iter), end_(end)
    {}

    virtual ObjectPtr key() const {
        return iter_->first()->as_object();
    }

    virtual ObjectPtr value() const {
        return iter_->second();
    }

    virtual bool is_end() const {
        return iter_ == end_;
    }

    virtual void next() {
        iter_++;
    }
};


template <>
class TypedGenericMap<Varchar, Object>: public GenericMap, public pool::enable_shared_from_this<TypedGenericMap<Varchar, Object>> {
    ViewPtrHolder* ctr_holder_;
    mutable Map<Varchar, Object> map_;
public:
    TypedGenericMap(void* map, HermesCtr* ctr, ViewPtrHolder* ctr_holder):
        ctr_holder_(ctr_holder),
        map_(map, ctr, ctr_holder)
    {
        ctr_holder->ref_copy();
    }

    virtual ~TypedGenericMap() noexcept {
        ctr_holder_->unref();
    }

    virtual uint64_t size() const {
        return map_.size();
    }

    virtual ObjectPtr get(const ObjectPtr& key) const {
        return map_.get(key->as_varchar()->view());
    }

    virtual void put(const ObjectPtr& key, const ObjectPtr& value) {
        map_.put(key->as_varchar(), value);
    }


    virtual void remove(const ObjectPtr& key) {
        map_.remove(key->as_varchar()->view());
    }

    virtual PoolSharedPtr<HermesCtr> ctr() const {
        return map_.document();
    }

    virtual bool is_array() const {
        return false;
    }

    virtual bool is_map() const {
        return true;
    }

    virtual bool empty() const {
        return map_.size() == 0;
    }

    virtual PoolSharedPtr<GenericArray> as_array() const {
        MEMORIA_MAKE_GENERIC_ERROR("Can't convert Key<Varchar, Object> to GenericArray").do_throw();
    }

    virtual PoolSharedPtr<GenericMap> as_map() const {
        return this->shared_from_this();
    }

    virtual ObjectPtr as_object() const {
        return map_.as_object();
    }

    virtual PoolSharedPtr<GenericMapEntry> iterator() const;

    static PoolSharedPtr<GenericMap> make_wrapper(void* map, HermesCtr* ctr, ViewPtrHolder* ctr_holder);
};



}}
