
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

#include <memoria/core/strings/string.hpp>
#include <memoria/core/tools/arena_buffer.hpp>
#include <memoria/core/memory/object_pool.hpp>

#include <memoria/core/hermes/traits.hpp>

#include <memoria/core/arena/arena.hpp>

#include <ostream>

namespace memoria {
namespace hermes {


class HermesCtr;

class CtrAware {
protected:
    mutable HermesCtr* ctr_;
public:
    CtrAware(HermesCtr* ctr): ctr_(ctr) {}

    PoolSharedPtr<HermesCtr> ctr() const;
};

class StringifySpec {
    const char* space_;

    const char* nl_start_;
    const char* nl_middle_;
    const char* nl_end_;

    size_t indent_size_;
public:
    StringifySpec(): StringifySpec(" ", "\n", "\n", "\n", 2) {}

    StringifySpec(
            const char* space,
            const char* nl_start,
            const char* nl_middle,
            const char* nl_end,
            size_t indent_size
    ):
        space_(space),
        nl_start_(nl_start),
        nl_middle_(nl_middle),
        nl_end_(nl_end),
        indent_size_(indent_size)
    {}


    static StringifySpec no_indent() {
        return StringifySpec("", "", "", "", 0);
    }

    static StringifySpec simple() {
        return StringifySpec("", "", " ", "", 0);
    }

    const char* space() const {return space_;}

    const char* nl_start() const {return nl_start_;}
    const char* nl_middle() const {return nl_middle_;}
    const char* nl_end() const {return nl_end_;}

    size_t indent_size() const {return indent_size_;}
};

class StringifyCfg {
    bool use_raw_strings_{true};
    StringifySpec spec_{StringifySpec::no_indent()};

public:
    StringifyCfg() {}

    bool use_raw_strings() const {return use_raw_strings_;}

    StringifyCfg& with_raw_strings(bool val) {
        use_raw_strings_ = val;
        return *this;
    }

    const StringifySpec& spec() const {
        return spec_;
    }

    StringifyCfg& set_spec(const StringifySpec& spec) {
        spec_ = spec;
        return *this;
    }

    StringifyCfg& with_spec(const StringifySpec& spec) {
        spec_ = spec;
        return *this;
    }

    static StringifyCfg pretty() {
        StringifyCfg cfg;
        cfg.set_spec(StringifySpec());
        return cfg;
    }

    static StringifyCfg simple() {
        StringifyCfg cfg;
        cfg.set_spec(StringifySpec::simple());
        return cfg;
    }


    static StringifyCfg pretty1() {
        return pretty().with_raw_strings(false);
    }
};

class DumpFormatState {
    const StringifyCfg& cfg_;
    size_t current_indent_;
public:
    DumpFormatState(const StringifyCfg& cfg):
        cfg_(cfg),
        current_indent_(0)
    {}

    DumpFormatState(const StringifyCfg& cfg, size_t indent):
        cfg_(cfg),
        current_indent_(indent)
    {}

    const StringifyCfg& cfg() const {return cfg_;}

    size_t current_indent() const {return current_indent_;}

    void push() {
        current_indent_ += cfg_.spec().indent_size();
    }

    void pop() {
        current_indent_ -= cfg_.spec().indent_size();
    }

    void make_indent(std::ostream& out) const {
        auto space = cfg_.spec().space();
        for (size_t c = 0; c < current_indent_; c++) {
            out << space;
        }
    }
};



class RawStringEscaper {
    ArenaBuffer<U8StringView::value_type> buffer_;
public:

    bool has_quotes(U8StringView str) const noexcept
    {
        for (auto& ch: str) {
            if (ch == '\'') return true;
        }

        return false;
    }

    U8StringView escape_quotes(const U8StringView& str)
    {
        if (!has_quotes(str)) {
            return str;
        }
        else {
            buffer_.clear();

            for (auto& ch: str)
            {
                if (MMA_UNLIKELY(ch == '\''))
                {
                    buffer_.append_value('\\');
                }

                buffer_.append_value(ch);
            }

            buffer_.append_value(0);

            return U8StringView(buffer_.data(), buffer_.size() - 1);
        }
    }

    void reset()
    {
        if (buffer_.size() <= 1024*16) {
            buffer_.clear();
        }
        else {
            buffer_.reset();
        }
    }

    static RawStringEscaper& current();
};


class StringEscaper {
    ArenaBuffer<U8StringView::value_type> buffer_;
public:

    bool has_escaping_char(U8StringView str) const noexcept
    {
        for (auto& ch: str) {
            switch (ch) {
                case '\b':
                case '\f':
                case '\r':
                case '\n':
                case '\t':
                case '\"':
                case '\\': return true;
            }
        }

        return false;
    }

    U8StringView escape_chars(const U8StringView& str)
    {
        if (!has_escaping_char(str)) {
            return str;
        }
        else {
            buffer_.clear();

            for (auto& ch: str)
            {
                switch(ch) {
                    case '\b': buffer_.append_values(Span<const char>("\\b", 2)); continue;
                    case '\f': buffer_.append_values(Span<const char>("\\f", 2)); continue;
                    case '\r': buffer_.append_values(Span<const char>("\\r", 2)); continue;
                    case '\n': buffer_.append_values(Span<const char>("\\n", 2)); continue;
                    case '\t': buffer_.append_values(Span<const char>("\\t", 2)); continue;
                    case '\"': buffer_.append_values(Span<const char>("\\\"", 2)); continue;
                    case '\\': buffer_.append_values(Span<const char>("\\\\", 2)); continue;
                }

                buffer_.append_value(ch);
            }

            buffer_.append_value(0);

            return U8StringView(buffer_.data(), buffer_.size() - 1);
        }
    }

    void reset()
    {
        if (buffer_.size() <= 1024*16) {
            buffer_.clear();
        }
        else {
            buffer_.reset();
        }
    }

    static StringEscaper& current();
};




template <typename AccessorType>
class RandomAccessIterator: public boost::iterator_facade<
        RandomAccessIterator<AccessorType>,
        const typename AccessorType::ViewType,
        std::random_access_iterator_tag,
        const typename AccessorType::ViewType
> {
    using ViewType = typename AccessorType::ViewType;

    size_t pos_;
    size_t size_;
    AccessorType accessor_;

    using Iterator = RandomAccessIterator;

public:
    RandomAccessIterator() : pos_(), size_(), accessor_() {}

    RandomAccessIterator(AccessorType accessor, size_t pos, size_t size) :
        pos_(pos), size_(size), accessor_(accessor)
    {}

    size_t size() const noexcept {return size_;}
    size_t pos() const noexcept {return pos_;}

    bool is_end() const noexcept {return pos_ >= size_;}
    operator bool() const noexcept {return !is_end();}

private:
    friend class boost::iterator_core_access;

    ViewType dereference() const  {
        return accessor_.get(pos_);
    }

    bool equal(const RandomAccessIterator& other) const  {
        return accessor_ == other.accessor_ && pos_ == other.pos_;
    }

    void increment() {
        pos_ += 1;
    }

    void decrement() {
        pos_ -= 1;
    }

    void advance(int64_t n)  {
        pos_ += n;
    }

    ptrdiff_t distance_to(const RandomAccessIterator& other) const
    {
        ptrdiff_t res = static_cast<ptrdiff_t>(other.pos_) - static_cast<ptrdiff_t>(pos_);
        return res;
    }
};


template <typename AccessorType>
class ForwardIterator: public boost::iterator_facade<
        ForwardIterator<AccessorType>,
        const typename AccessorType::ViewType,
        std::forward_iterator_tag,
        const typename AccessorType::ViewType
> {
    using ViewType = typename AccessorType::ViewType;

    AccessorType accessor_;

    using Iterator = ForwardIterator;

public:
    ForwardIterator() : accessor_() {}

    ForwardIterator(AccessorType accessor) : accessor_(accessor)
    {}

private:
    friend class boost::iterator_core_access;

    ViewType dereference() const  {
        return accessor_.current();
    }

    bool equal(const ForwardIterator& other) const  {
        return accessor_ == other.accessor_;
    }

    void increment() {
        accessor_.next();
    }
};



class TaggedValue {
    static constexpr size_t VALUE_ALIGN = 8;
    static constexpr size_t VALUE_SIZE  = 8;

private:

    ShortTypeCode tag_;
    alignas(VALUE_ALIGN) uint8_t value_[VALUE_SIZE];

public:
    TaggedValue() = default;

    template <typename T>
    TaggedValue(ShortTypeCode tag, T value):
        tag_(tag)
    {
        set(value);
    }

    template <typename T>
    void set(const T& vv) {
        static_assert(fits_in<T>(), "");
        *reinterpret_cast<T*>(value_) = vv;
    }

    template <typename T>
    T& get() {
        static_assert(fits_in<T>(), "");
        return *reinterpret_cast<T*>(value_);
    }

    template <typename T>
    const T& get() const {
        static_assert(fits_in<T>(), "");
        return *reinterpret_cast<const T*>(value_);
    }

    template <typename T>
    T& get_unchecked() {
        return *reinterpret_cast<T*>(value_);
    }

    template <typename T>
    const T& get_unchecked() const {
        return *reinterpret_cast<const T*>(value_);
    }

    const ShortTypeCode& tag() const {
        return tag_;
    }

    template <typename T>
    static constexpr bool fits_in()
    {
        return alignof(T) <= VALUE_ALIGN && sizeof(T) <= VALUE_SIZE;
    }

    template <typename DT>
    static constexpr bool dt_fits_in()
    {
        using T = DTTViewType<DT>;
        return DataTypeTraits<DT>::isFixedSize && fits_in<T>();
    }
};

struct TaggedGenericView: SharedPtrHolder {
    static constexpr size_t VIEW_ALIGN_OF = 8;
    static constexpr size_t MAX_VIEW_SIZE = 64;

private:
    ShortTypeCode tag_;
    void* view_ptr_;

public:
    TaggedGenericView() = default;
    TaggedGenericView(ShortTypeCode tag):
        tag_(tag), view_ptr_()
    {}

    const ShortTypeCode& tag() const {
        return tag_;
    }

    template <typename T>
    T& get() {
        return *reinterpret_cast<T*>(view_ptr_);
    }

    void set_buffer(void* buffer) {
        view_ptr_ = buffer;
    }

    static PoolSharedPtr<TaggedGenericView> allocate_space(size_t size);

    template <typename DT>
    static PoolSharedPtr<TaggedGenericView> allocate(DTTViewType<DT> view)
    {
        using ViewT = DTTViewType<DT>;
        static_assert(alignof(ViewT) <= VIEW_ALIGN_OF && sizeof(view) < MAX_VIEW_SIZE);

        auto ptr = allocate_space(sizeof(view));
        new (ptr->view_ptr_) ViewT(view);
        return ptr;
    }
};

enum ValueStorageTag {
    VS_TAG_ADDRESS, VS_TAG_SMALL_VALUE, VS_TAG_GENERIC_VIEW
};

union ValueStorage {
    void* addr;
    TaggedValue small_value;
    TaggedGenericView* view_ptr;

    template <typename DT>
    DTTViewType<DT>& get_view(ValueStorageTag tag)
    {
        if (tag == ValueStorageTag::VS_TAG_SMALL_VALUE) {
            return small_value.get_unchecked<DTTViewType<DT>>();
        }
        else if (tag == ValueStorageTag::VS_TAG_GENERIC_VIEW) {
            if (view_ptr) {
                return view_ptr->get<DTTViewType<DT>>();
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Provided ValueStorage GenericView is null").do_throw();
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Unsupported ValueStorageTag value: {}", (int64_t)tag).do_throw();
        }
    }

    template <typename DT>
    const DTTViewType<DT>& get_view(ValueStorageTag tag) const
    {
        if (tag == ValueStorageTag::VS_TAG_SMALL_VALUE) {
            return small_value.get_unchecked<DTTViewType<DT>>();
        }
        else if (tag == ValueStorageTag::VS_TAG_GENERIC_VIEW) {
            if (view_ptr) {
                return view_ptr->get<DTTViewType<DT>>();
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Unsupported ValueStorageTag value: {}", (int64_t)tag).do_throw();
        }
    }
};

static inline ShortTypeCode get_type_tag(ValueStorageTag vs_tag, const ValueStorage& value_storage) noexcept
{
    if (vs_tag == ValueStorageTag::VS_TAG_ADDRESS) {
        return arena::read_type_tag(value_storage.addr);
    }
    else if (vs_tag == ValueStorageTag::VS_TAG_SMALL_VALUE) {
        return value_storage.small_value.tag();
    }
    else if (vs_tag == ValueStorageTag::VS_TAG_GENERIC_VIEW) {
        if (value_storage.view_ptr) {
            return value_storage.view_ptr->tag();
        }
    }

    return ShortTypeCode::nullv();
}

hermes::DatatypePtr strip_namespaces(hermes::DatatypePtr datatype);

namespace detail {

template <typename T, bool HasResetMethod = memoria::pool::detail::ObjectPoolLicycleMethods<T>::HasSetBuffer>
struct SetObjectBufferHelper {
    static void process(T*, void*) noexcept {}
};

template <typename T>
struct SetObjectBufferHelper<T, true> {
    static void process(T* obj, void* buffer) noexcept {
        obj->set_buffer(buffer);
    }
};


}


template <size_t AlignOf, size_t Size, typename ValueT>
class AlignedSpacePool: public pool::PoolBase, public boost::enable_shared_from_this<AlignedSpacePool<AlignOf, Size, ValueT>> {
public:

    virtual U8String pool_type() {
        return TypeNameFactory<AlignedSpacePool>::name();
    }

    class RefHolder: public ValueT {
        alignas (AlignOf) std::byte object_storage_[Size];

        boost::local_shared_ptr<AlignedSpacePool> pool_;

        template <typename> friend class SharedPtr;
        template <typename> friend class UniquePtr;

        template <size_t, size_t, typename>
        friend class AlignedSpacePool;



    public:
        template <typename... Args>
        RefHolder(boost::local_shared_ptr<AlignedSpacePool> pool, Args&&... args):
            ValueT(std::forward<Args>(args)...),
            pool_(std::move(pool))
        {
            detail::SetObjectBufferHelper<ValueT>::process(this, object_storage_);
        }

        template <typename T>
        T* ptr() noexcept {
            static_assert(sizeof(T) <= Size, "");
            return std::launder(reinterpret_cast<T*>(object_storage_));
        }

        virtual ~RefHolder() noexcept  = default;

    protected:
        virtual void dispose() noexcept {
            pool_->release(this);
        }

        virtual void destroy() noexcept {
        }
    };

private:

    boost::object_pool<RefHolder> alloc_;

    friend class RefHolder;
    template <typename> friend class SharedPtr;
    template <typename> friend class UniquePtr;
public:

    template <typename... Args>
    pool::SharedPtr<ValueT> allocate_shared(Args&&... args)
    {
        auto ptr = new (alloc_.malloc()) RefHolder(this->shared_from_this(), std::forward<Args>(args)...);
        pool::detail::SharedFromThisHelper<ValueT>::initialize(ptr, ptr);
        return pool::detail::make_shared_ptr_from(ptr, ptr);
    }

    template <typename... Args>
    UniquePtr<ValueT> allocate_unique(Args&&... args)
    {
        auto ptr = new (alloc_.malloc()) RefHolder(this->shared_from_this(), std::forward<Args>(args)...);
        return pool::detail::make_unique_ptr_from(ptr, ptr);
    }

private:
    void release(RefHolder* holder) noexcept {
        alloc_.destroy(holder);
    }
};




class TaggedHoldingView {
    static constexpr size_t TAG_MASK = 0x7;
    mutable size_t ptr_holder_;

    template <typename, bool>
    friend class memoria::ViewPtr;

public:
    TaggedHoldingView() noexcept:
        ptr_holder_()
    {
    }

    TaggedHoldingView(ViewPtrHolder* holder) noexcept:
        ptr_holder_(reinterpret_cast<size_t>(holder))
    {}


protected:
    ViewPtrHolder* get_ptr_holder() const noexcept {
        return reinterpret_cast<ViewPtrHolder*>(ptr_holder_ & ~TAG_MASK);
    }

    void reset_ptr_holder() noexcept {
        ptr_holder_ = 0;
    }

    size_t get_tag() const noexcept {
        return ptr_holder_ & TAG_MASK;
    }

    // Only last 3 bits of the tag are counted
    void set_tag(size_t tag) noexcept {
        ptr_holder_ &= ~TAG_MASK;
        ptr_holder_ |= (tag & TAG_MASK);
    }
};


template <typename List>
struct IsCodeInTheList;

template <typename T, typename... Tail>
struct IsCodeInTheList<TL<T, Tail...>> {
    static bool is_in(ShortTypeCode code) noexcept {
        return ShortTypeCode::of<T>() == code || IsCodeInTheList<TL<Tail...>>::is_in(code);
    }
};

template <>
struct IsCodeInTheList<TL<>> {
    static bool is_in(ShortTypeCode code) noexcept {
        return false;
    }
};


namespace detail {

template <typename T>
struct ValueCastHelper {
    static ViewPtr<T> cast_to(ValueStorageTag, ValueStorage& storage, HermesCtr* doc, ViewPtrHolder* ref_holder) noexcept {
        return ViewPtr<T>(T(
            storage.addr,
            doc,
            ref_holder
        ));
    }
};

}

struct GenericArray;
struct GenericMap;

struct GenericObject {
    virtual ~GenericObject() noexcept = default;
    virtual PoolSharedPtr<HermesCtr> ctr() const = 0;

    virtual bool is_array() const = 0;
    virtual bool is_map()   const = 0;

    virtual PoolSharedPtr<GenericArray> as_array() const = 0;
    virtual PoolSharedPtr<GenericMap> as_map() const = 0;
    virtual ObjectPtr as_object() const = 0;
};


using GenericArrayPtr   = PoolSharedPtr<GenericArray>;
using GenericMapPtr     = PoolSharedPtr<GenericMap>;
using GenericObjectPtr  = PoolSharedPtr<GenericObject>;

class HermesASTNode {
    U8StringView name_;
    uint64_t code_;
public:
    constexpr HermesASTNode(uint64_t code):
        name_(), code_(code)
    {}

    constexpr HermesASTNode(U8StringView name, uint64_t code):
        name_(name), code_(code)
    {}

    constexpr U8StringView name() const {
        return name_;
    }

    constexpr uint64_t code() const {
        return code_;
    }

    constexpr bool is_anonymous() const {
        return name_.size() == 0;
    }
};

}}
