
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
class MapView<Varchar, Object>: public HoldingView<MapView<Varchar, Object>> {
    using Base = HoldingView<MapView<Varchar, Object>>;
public:
    using KeyT = DataObjectView<Varchar>::ArenaDTContainer;

    using KeyPtrT = arena::RelativePtr<KeyT>;
    using ValuePtrT = arena::EmbeddingRelativePtr<void>;

    using MapStorageT = arena::Map<KeyPtrT, ValuePtrT>;

    static_assert(std::is_standard_layout_v<MapStorageT>,"");

protected:
    mutable MapStorageT* map_;
    using Base::mem_holder_;

    friend class HermesCtr;
    friend class ObjectView;

    template <typename, typename>
    friend class TypedGenericMap;

    template <typename, typename>
    friend class MapView;

    template <typename>
    friend class ArrayView;

    friend class HermesCtrBuilder;
    friend class memoria::hermes::path::interpreter::Interpreter;
    using MapIterator = typename MapStorageT::Iterator;


    class EntryT {
        const MapIterator* iter_;
        mutable LWMemHolder* mem_holder_;

    public:
        EntryT(const MapIterator* iter, LWMemHolder* ptr_holder):
            iter_(iter), mem_holder_(ptr_holder)
        {}

        StringValue first() const {
            return StringValue(StringValueView(mem_holder_, iter_->key().get()));
        }

        Object second() const
        {
            if (iter_->value().is_not_null()) {
                auto ptr = iter_->value().get();
                return Object(mem_holder_, ptr);
            }
            else {
                return Object(mem_holder_, nullptr);
            }
        }
    };

    using Accessor = MapIteratorAccessor<EntryT, MapView, MapIterator>;

public:
    using Iterator = ForwardIterator<Accessor>;

public:
    MapView() noexcept : map_() {}

    MapView(LWMemHolder* ptr_holder, void* map) noexcept :
        Base(ptr_holder),
        map_(reinterpret_cast<MapStorageT*>(map))
    {}

    bool is_null() const noexcept {
        return !map_;
    }

    bool is_not_null() const noexcept {
        return map_;
    }

    ObjectMap self() const {
        return ObjectMap(MapView(mem_holder_, map_));
    }

    Iterator begin() const {
        return Iterator(Accessor(self(), map_->begin(), mem_holder_));
    }

    Iterator end() const {
        return Iterator(Accessor(self(), map_->end(), mem_holder_));
    }

    Iterator cbegin() const {
        return Iterator(Accessor(self(), map_->begin(), mem_holder_));
    }

    Iterator cend() const {
        return Iterator(Accessor(self(), map_->end(), mem_holder_));
    }

    PoolSharedPtr<HermesCtr> document() const {
        assert_not_null();
        return PoolSharedPtr<HermesCtr>(
                    mem_holder_->ctr(),
                    mem_holder_->owner(),
                    pool::DoRef{}
        );
    }

    Object as_object() const {
        return Object(ObjectView(mem_holder_, map_));
    }

    uint64_t size() const {
        assert_not_null();
        return map_->size();
    }

    bool empty() const {
        return size() == 0;
    }

    Object get(U8StringView key) const
    {
        assert_not_null();
        const ValuePtrT* res = map_->get(key);

        if (res)
        {
            if (MMA_LIKELY(res->is_pointer()))
            {
                if (MMA_LIKELY(res->is_not_null())) {
                    return Object(ObjectView(mem_holder_, res->get()));
                }
                else {
                    return Object{};
                }
            }
            else {
                TaggedValue tv(*res);
                return Object(ObjectView(mem_holder_, tv));
            }
        }
        else {
            return Object{};
        }
    }

    template <typename T>
    Object get(const NamedTypedCode<T>& code) const {
        return get(code.name());
    }

    template <typename DT>
    ObjectMap put_dataobject(U8StringView key, DTTViewType<DT> value);

    template <typename DT, typename T>
    ObjectMap put_dataobject(const NamedTypedCode<T>& code, DTTViewType<DT> value) {
        return put_dataobject<DT>(code.name(), value);
    }

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


    void for_each(std::function<void(U8StringView, Object)> fn) const
    {
        assert_not_null();
        map_->for_each([&](const auto& key, const auto& value){
            U8StringView kk = key->view();

            if (value.is_pointer())
            {
                if (value.is_not_null()) {
                    fn(kk, Object(ObjectView(mem_holder_, value.get())));
                }
                else {
                    fn(kk, Object(ObjectView()));
                }
            }
            else {
                TaggedValue tv(value);
                fn(kk, Object(ObjectView(mem_holder_, tv)));
            }
        });
    }

    bool is_simple_layout() const noexcept
    {
        if (size() > 1) {
            return false;
        }

        bool simple = true;

        for_each([&](auto, auto vv){
            simple = simple && vv->is_simple_layout();
        });

        return simple;
    }

    ObjectMap remove(U8StringView key);

    void* deep_copy_to(arena::ArenaAllocator& arena, DeepCopyDeduplicator& dedup) const {
        assert_not_null();
        return map_->deep_copy_to(arena, ShortTypeCode::of<Map<Varchar, Object>>(), mem_holder_, dedup);
    }

    PoolSharedPtr<GenericMap> as_generic_map() const;

    // FIXME: const Object&
    ObjectMap put(StringValue name, Object value);
    ObjectMap put(U8StringView name, Object value);

    template <typename T>
    ObjectMap put(const NamedTypedCode<T>& code, const Object& value) {
        return put(code.name(), value);
    }

    operator Object() const & noexcept {
        return as_object();
    }

    operator Object() && noexcept {
        return Object(this->release_mem_holder(), map_, MoveOwnershipTag{});
    }

private:
    void do_stringify(std::ostream& out, DumpFormatState& state) const;

    void assert_not_null() const
    {
        if (MMA_UNLIKELY(map_ == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("MapView<Varchar, ObjectView> is null").do_throw();
        }
    }

    void assert_mutable();
};


template <>
class TypedGenericMapEntry<Varchar, Object>: public GenericMapEntry {
    using MapT = MapView<Varchar, Object>;
    using IteratorT = typename MapT::Iterator;

    IteratorT iter_;
    IteratorT end_;

public:
    TypedGenericMapEntry(IteratorT iter, IteratorT end):
        iter_(iter), end_(end)
    {}

    virtual Object key() const {
        return iter_->first()->as_object();
    }

    virtual Object value() const {
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
    LWMemHolder* ctr_holder_;
    mutable MapView<Varchar, Object> map_;
public:
    TypedGenericMap(LWMemHolder* ctr_holder, void* map):
        ctr_holder_(ctr_holder),
        map_(ctr_holder, map)
    {
        ctr_holder->ref_copy();
    }

    virtual ~TypedGenericMap() noexcept {
        ctr_holder_->unref();
    }

    virtual uint64_t size() const {
        return map_.size();
    }

    virtual Object get(const Object& key) const {
        return map_.get(*key->as_varchar()->view());
    }

    virtual Object get(U8StringView key) const {
        return map_.get(key);
    }

    virtual Object get(int32_t key) const {
        MEMORIA_MAKE_GENERIC_ERROR("Method map(int32_t) is not supported by this generic map").do_throw();
    }

    virtual Object get(uint64_t key) const {
        MEMORIA_MAKE_GENERIC_ERROR("Method map(uint64_t) is not supported by this generic map").do_throw();
    }

    virtual Object get(uint8_t key) const {
        MEMORIA_MAKE_GENERIC_ERROR("Method map(uint8_t) is not supported by this generic map").do_throw();
    }




    virtual GenericMapPtr put(const Object& key, const Object& value) {
        auto new_map = map_.put(key->as_varchar(), value);
        return make_wrapper(ctr_holder_, new_map->map_);
    }


    virtual GenericMapPtr remove(const Object& key) {
        auto new_map = map_.remove(*key->as_varchar()->view());
        return make_wrapper(ctr_holder_, new_map->map_);
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
        MEMORIA_MAKE_GENERIC_ERROR("Can't convert Key<Varchar, ObjectView> to GenericArray").do_throw();
    }

    virtual PoolSharedPtr<GenericMap> as_map() const {
        return this->shared_from_this();
    }

    virtual Object as_object() const {
        return map_.as_object();
    }

    virtual PoolSharedPtr<GenericMapEntry> iterator() const;

    static PoolSharedPtr<GenericMap> make_wrapper(LWMemHolder* ctr_holder, void* map);
};



}}
