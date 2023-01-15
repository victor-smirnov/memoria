
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
    using KeyT = arena::ArenaDataTypeContainer<Varchar>;

    using KeyPtrT = arena::RelativePtr<KeyT>;
    using ValuePtrT = arena::EmbeddingRelativePtr<void>;

    using MapStorageT = arena::Map<KeyPtrT, ValuePtrT>;

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



        StringOView first() const {
            return iter_->key()->view(holder_.holder());
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

    using Accessor = MapIteratorAccessor<EntryT, Map<Varchar, Object>, MapIterator>;

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
        return MemHolderHandle(get_mem_holder());
    }

    bool is_null() const noexcept {
        return !map_;
    }

    bool is_not_null() const noexcept {
        return map_;
    }

    ObjectMap self() const {
        return ObjectMap(mem_holder_, map_);
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

    Object get(U8StringView key) const
    {
        assert_not_null();
        const ValuePtrT* res = map_->get(key, get_mem_holder());

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
            auto kk = key->view(mem_holder_);

            if (value.is_pointer())
            {
                if (value.is_not_null()) {
                    fn(kk, Object(mem_holder_, value.get()));
                }
                else {
                    fn(kk, Object());
                }
            }
            else {
                TaggedValue tv(value);
                fn(kk, Object(mem_holder_, tv));
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

    MMA_NODISCARD ObjectMap remove(U8StringView key);

    void* deep_copy_to(DeepCopyState& dedup) const {
        assert_not_null();
        return map_->deep_copy_to(ShortTypeCode::of<Map<Varchar, Object>>(), dedup);
    }

    PoolSharedPtr<GenericMap> as_generic_map() const;

    template <typename V, std::enable_if_t<HermesObject<V>::Value, int> = 0>
    MMA_NODISCARD ObjectMap put(U8StringView name, const V& value) {
        return put_object(name, value);
    }

    template <typename ViewT, std::enable_if_t<!HermesObject<ViewT>::Value, int> = 0>
    MMA_NODISCARD ObjectMap put(U8StringView name, const ViewT& value) {
        using DT = typename ViewToDTMapping<ViewT>::Type;
        return put_dataobject<DT>(name, value);
    }

    template <typename DT, typename ViewT>
    MMA_NODISCARD ObjectMap put_t(U8StringView name, const ViewT& view) {
        return put_dataobject<DT>(name, view);
    }

    template <typename T, typename V>
    MMA_NODISCARD ObjectMap put(const NamedTypedCode<T>& code, const V& value) {
        return put(code.name(), value);
    }

    template <typename DT, typename T, typename ViewT>
    MMA_NODISCARD ObjectMap put_t(const NamedTypedCode<T>& code, const ViewT& value) {
        return put_t<DT>(code.name(), value);
    }

    operator Object() const & noexcept {
        return as_object();
    }

    operator Object() && noexcept {
        return Object(this->release_mem_holder(), map_, MoveOwnershipTag{});
    }

    bool operator!=(const MapView<Varchar, Object>& other) const {
        return !operator==(other);
    }

    bool operator==(const MapView<Varchar, Object>& other) const
    {
        if (is_not_null() && other.is_not_null())
        {
            uint64_t my_size = size();
            if (my_size == other.size())
            {
                auto ii = begin();
                auto ee = end();

                while (ii != ee) {
                    Object vv = other.get(ii->first());

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
        map->check_object_map(state);
    }


private:
    MMA_NODISCARD ObjectMap put_object(U8StringView name, const Object& value);

    template <typename DT>
    MMA_NODISCARD ObjectMap put_dataobject(U8StringView key, const DTTViewType<DT>& value);

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
        return Object(
            iter_.accessor().map().mem_holder().holder(),
            iter_->first(),
            DirectViewTag<Varchar>{}
        );
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
    mutable Map<Varchar, Object> map_;
public:
    TypedGenericMap(Map<Varchar, Object>&& map):
        map_(std::move(map))
    {
    }

    virtual uint64_t size() const {
        return map_.size();
    }

    virtual uint64_t capacity() const {
        return map_.capacity();
    }

    virtual Object get(const Object& key) const {
        return map_.get(key.as_varchar());
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
        auto new_map = map_.put(key.as_varchar(), value);
        return make_wrapper(std::move(new_map));
    }


    virtual GenericMapPtr remove(const Object& key) {
        auto new_map = map_.remove(key.as_varchar());
        return make_wrapper(std::move(new_map));
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
        MEMORIA_MAKE_GENERIC_ERROR("Can't convert Key<Varchar, ObjectView> to GenericArray").do_throw();
    }

    virtual PoolSharedPtr<GenericMap> as_map() const {
        return this->shared_from_this();
    }

    virtual Object as_object() const {
        return map_.as_object();
    }

    virtual PoolSharedPtr<GenericMapEntry> iterator() const;

    static PoolSharedPtr<GenericMap> make_wrapper(Map<Varchar, Object>&&);
};



}}
