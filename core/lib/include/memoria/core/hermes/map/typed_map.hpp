
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

template <typename KeyDT>
class TypedMapData<KeyDT, Object, FSEKeySubtype, true>: public arena::Map<DTTViewType<KeyDT>, arena::EmbeddingRelativePtr<void>> {
    using Base = arena::Map<DTTViewType<KeyDT>, arena::EmbeddingRelativePtr<void>>;
public:
    TypedMapData() {}
    TypedMapData(uint64_t capacity):
        Base(capacity)
    {}
};


template <typename KeyDT>
class MapView<KeyDT, Object>: public HoldingView<MapView<KeyDT, Object>> {
    using Base = HoldingView<MapView<KeyDT, Object>>;
public:
    using KeyView     = DTTViewType<KeyDT>;
    using MapStorageT = TypedMapData<KeyDT, Object, FSEKeySubtype, true>;

    static_assert(std::is_standard_layout_v<MapStorageT>,"");

protected:
    mutable MapStorageT* map_;
    using Base::mem_holder_;

    friend class HermesCtrView;
    friend class ObjectView;

    template <typename, typename>
    friend class TypedGenericMap;

    template <typename, typename>
    friend class MapView;

    template <typename>
    friend class ArrayView;

    friend class HermesCtrBuilder;
    using MapIterator = typename MapStorageT::Iterator;

    class EntryT {
        const MapIterator* iter_;
        MemHolderHandle holder_;

    public:
        EntryT(const MapIterator* iter, MemHolderHandle&& holder):
            iter_(iter), holder_(std::move(holder))
        {}

        KeyView first() const {
            return iter_->key();
        }

        Object second() const
        {
            if (iter_->value().is_not_null()) {
                auto ptr = iter_->value().get();
                return Object(holder_.holder(), ptr);
            }
            else {
                return Object();
            }
        }
    };

    using Accessor = MapIteratorAccessor<EntryT, Map<KeyDT, Object>, MapIterator>;

public:
    using Iterator = ForwardIterator<Accessor>;

public:
    MapView() noexcept : map_() {}

    MapView(LWMemHolder* ptr_holder, void* map) noexcept :
        Base(ptr_holder),
        map_(reinterpret_cast<MapStorageT*>(map))
    {}

    MemHolderHandle mem_holder() const {
        assert_not_null();
        return MemHolderHandle(this->get_mem_holder());
    }

    bool is_null() const noexcept {
        return !map_;
    }

    bool is_not_null() const noexcept {
        return map_;
    }

    Map<KeyDT, Object> self() const {
        return Map<KeyDT, Object>(mem_holder_, map_);
    }

    Iterator begin() const {
        return Iterator(Accessor(self(), map_->begin()));
    }

    Iterator end() const {
        return Iterator(Accessor(self(), map_->end()));
    }

    Iterator cbegin() const {
        return Iterator(Accessor(self(), map_->begin()));
    }

    Iterator cend() const {
        return Iterator(Accessor(self(), map_->end()));
    }

    HermesCtr ctr() const;

    Object as_object() const {
        return Object(mem_holder_, map_);
    }

    uint64_t size() const {
        assert_not_null();
        return map_->size();
    }

    uint64_t capacity() const {
        assert_not_null();
        return map_->capacity();
    }

    bool empty() const {
        return size() == 0;
    }

    Object get(KeyView key) const
    {
        assert_not_null();
        const auto* res = map_->get(key, mem_holder_);
        if (res)
        {
            if (MMA_LIKELY(res->is_pointer()))
            {
                if (MMA_LIKELY(res->is_not_null())) {
                    return Object(mem_holder_, res->get());
                }
                else {
                    return Object{};
                }
            }
            else {
                TaggedValue tv(*res);
                return Object(mem_holder_, tv);
            }
        }
        else {
            return Object{};
        }
    }

    template <typename T>
    Object get(const NamedTypedCode<T>& code) const {
        return get(code.code());
    }



    template <typename V, std::enable_if_t<HermesObject<V>::Value, int> = 0>
    void put(KeyView name, const V& value) {
        return put_object(name, value);
    }

    template <typename ViewT, std::enable_if_t<!HermesObject<ViewT>::Value, int> = 0>
    void put(KeyView name, const ViewT& value) {
        using DT = typename ViewToDTMapping<ViewT>::Type;
        return put_dataobject<DT>(name, value);
    }


    template <typename DT, typename ViewT>
    void put_t(KeyView name, const ViewT& value) {
        return put_dataobject<DT>(name, value);
    }


    template <typename T, typename V>
    void put(const NamedTypedCode<T>& code, const V& value) {
        return put(code.code(), value);
    }

    template <typename DT, typename T, typename ViewT>
    void put_t(const NamedTypedCode<T>& code, const ViewT& view) {
        return put_t<DT>(code.code(), view);
    }


    void remove(KeyView key);

    template <typename T>
    void remove(const NamedTypedCode<T>& code) {
        return remove(code.code());
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


    void for_each(std::function<void(KeyView, Object)> fn) const
    {
        assert_not_null();
        map_->for_each([&](const auto& key, const auto& value){
            if (value.is_pointer())
            {
                if (value.is_not_null()) {
                    fn(key, Object(mem_holder_, value.get()));
                }
                else {
                    fn(key, Object());
                }
            }
            else {
                TaggedValue tv(value);
                fn(key, Object(mem_holder_, tv));
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
            simple = simple && vv.is_simple_layout();
        });

        return simple;
    }


    void* deep_copy_to(DeepCopyState& dedup) const {
        assert_not_null();
        return map_->deep_copy_to(ShortTypeCode::of<Map<KeyDT, Object>>(), dedup);
    }

    PoolSharedPtr<GenericMap> as_generic_map() const;

    operator Object() const & noexcept {
        return as_object();
    }

    operator Object() && noexcept {
        return Object(this->release_mem_holder(), map_, MoveOwnershipTag{});
    }


    bool operator!=(const MapView<KeyDT, Object>& other) const {
        return !operator==(other);
    }

    bool operator==(const MapView<KeyDT, Object>& other) const
    {
        if (is_not_null() && other.is_not_null())
        {
            uint64_t my_size = size();
            if (my_size == other.size())
            {
                auto ii = begin();
                auto ee = end();

                while (ii != ee) {
                    auto vv = other.get(ii->first());

                    if (ii->second() != vv) {
                        return false;
                    }

                    ++ii;
                }

                return true;
            }
            else {
                return false;
            }
        }

        return is_null() && other.is_null();
    }

    static void check_structure(const void* addr, CheckStructureState& state)
    {
        state.check_alignment<MapStorageT>(addr, MA_SRC);

        const MapStorageT* map
                = reinterpret_cast<const MapStorageT*>(addr);
        map->check_typed_map(state);
    }

private:
    void put_object(KeyView name, const Object& value);

    template <typename DT>
    void put_dataobject(KeyView key, const DTTViewType<DT>& value);


    void do_stringify(std::ostream& out, DumpFormatState& state) const;

    void assert_not_null() const {
        if (MMA_UNLIKELY(map_ == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("MapView<KeyDT, Object> is null").do_throw();
        }
    }

    void assert_mutable();
};


template <typename KeyDT>
class TypedGenericMapEntry<KeyDT, Object>: public GenericMapEntry {
    using MapT = MapView<KeyDT, Object>;
    using IteratorT = typename MapT::Iterator;

    IteratorT iter_;
    IteratorT end_;

public:
    TypedGenericMapEntry(IteratorT iter, IteratorT end):
        iter_(iter), end_(end)
    {}

    virtual Object key() const {
        return Object(iter_.accessor().map().mem_holder(), iter_->first(), DirectViewTag<KeyDT>{});
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


template <typename KeyDT>
class TypedGenericMap<KeyDT, Object>: public GenericMap, public pool::enable_shared_from_this<TypedGenericMap<KeyDT, Object>> {
    mutable Map<KeyDT, Object> map_;
public:
    TypedGenericMap(Map<KeyDT, Object>&& map):
        map_(std::move(map))
    {}


    virtual uint64_t size() const {
        return map_.size();
    }

    virtual uint64_t capacity() const {
        return map_.capacity();
    }

    virtual Object get(const Object& key) const {
        return map_.get(key.convert_to<KeyDT>().template as_data_object<KeyDT>());
    }

    virtual Object get(U8StringView key) const {
        MEMORIA_MAKE_GENERIC_ERROR("Method map(U8StringView) is not supported by this generic map").do_throw();
    }

    virtual Object get(int32_t key) const {
        return map_.get(key);
    }

    virtual Object get(uint64_t key) const {
        return map_.get(key);
    }

    virtual Object get(uint8_t key) const {
        return map_.get(key);
    }

    virtual void put(const Object& key, const Object& value) {
        map_.put(key.convert_to<KeyDT>().template as_data_object<KeyDT>(), value);
    }


    virtual void remove(const Object& key) {
        map_.remove(key.convert_to<KeyDT>().template as_data_object<KeyDT>());
    }

    virtual HermesCtr ctr() const;

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
        MEMORIA_MAKE_GENERIC_ERROR("Can't convert Map<KeyDT, Object> to GenericMap").do_throw();
    }

    virtual PoolSharedPtr<GenericMap> as_map() const {
        return this->shared_from_this();
    }

    virtual Object as_object() const {
        return map_.as_object();
    }

    virtual PoolSharedPtr<GenericMapEntry> iterator() const;

    static PoolSharedPtr<GenericMap> make_wrapper(Map<KeyDT, Object>&& map);
};



}}
