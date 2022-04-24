
// Copyright 2013 Victor Smirnov
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

#include <memoria/core/types.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/tools/reflection.hpp>
#include <memoria/profiles/common/block_operations.hpp>

#include <memoria/core/tools/bitmap.hpp>

#include <memoria/core/packed/tools/packed_tools.hpp>

#include <memoria/core/memory/ptr_cast.hpp>

namespace memoria {

template <typename PkdStruct> struct PackedStructTraits;

template <typename PkdStruct>
using AccumType = typename PackedStructTraits<PkdStruct>::AccumType;

enum class PkdSearchType {SUM, MAX};

template <typename PkdStruct>
constexpr PkdSearchType PkdKeySearchType = PackedStructTraits<PkdStruct>::KeySearchType;

template <typename PkdStruct>
using PkdSearchKeyType = typename PackedStructTraits<PkdStruct>::SearchKeyType;

template <typename PkdStruct>
using PkdSearchKeyDataType = typename PackedStructTraits<PkdStruct>::SearchKeyDataType;

template <typename PkdStruct>
using PkdSearchKeyDataType = typename PackedStructTraits<PkdStruct>::SearchKeyDataType;

template <typename PkdStruct>
constexpr PackedDataTypeSize PkdStructSizeType = PackedStructTraits<PkdStruct>::DataTypeSize;

template <typename PkdStruct>
constexpr size_t PkdStructIndexes = PackedStructTraits<PkdStruct>::Indexes;


template <typename List> struct PackedListStructSizeType;

template <typename Head, typename... Tail>
struct PackedListStructSizeType<TL<Head, Tail...>> {
    static const PackedDataTypeSize HeadValue = PkdStructSizeType<Head>;

    static const PackedDataTypeSize Value =
            (HeadValue == PackedDataTypeSize::VARIABLE) ?
                    PackedDataTypeSize::VARIABLE :
                    PackedListStructSizeType<TL<Tail...>>::Value;
};

template <>
struct PackedListStructSizeType<TL<>> {
    static const PackedDataTypeSize Value = PackedDataTypeSize::FIXED;
};


template <PackedDataTypeSize... SizeTypes> struct PackedSizeTypeList;

template <PackedDataTypeSize Head, PackedDataTypeSize... Tail>
struct PackedSizeTypeList<Head, Tail...>
{
    static const PackedDataTypeSize Value =
            (Head == PackedDataTypeSize::VARIABLE) ?
                PackedDataTypeSize::VARIABLE :
                PackedSizeTypeList<Tail...>::Value;
};

template <>
struct PackedSizeTypeList<> {
    static const PackedDataTypeSize Value = PackedDataTypeSize::FIXED;
};


template <typename MyType, typename Base> class PackedAllocatorBase;

class PackedAllocator;

class PackedAllocatable {
protected:
    psize_t allocator_offset_;

    psize_t& allocator_offset() {return allocator_offset_;}

public:

    template <typename MyType, typename Base>
    friend class PackedAllocatorBase;

    friend class PackedAllocator;


    static constexpr uint32_t VERSION                  = 1;
    static constexpr size_t AlignmentBlock             = PackedAllocationAlignment;


    using FieldsList = TypeList<
            ConstValue<uint32_t, VERSION>,
            decltype(allocator_offset_)
    >;

    PackedAllocatable() noexcept = default;

    const psize_t& allocator_offset() const {return allocator_offset_;}

    void setTopLevelAllocator()
    {
        allocator_offset() = 0;
    }


    bool has_allocator() const
    {
        return allocator_offset_ > 0;
    }

    void set_allocator_offset(const void* allocator)
    {
        // TODO: check for UB.
        const char* my_ptr = ptr_cast<const char>(this);
        const char* alc_ptr = ptr_cast<const char>(allocator);
        ptrdiff_t diff = reinterpret_cast<ptrdiff_t>(my_ptr - alc_ptr);
        allocator_offset() = diff;
    }

    PackedAllocator* allocator()
    {
        if (allocator_offset() > 0)
        {
            uint8_t* my_ptr = ptr_cast<uint8_t>(this);
            return ptr_cast<PackedAllocator>(my_ptr - allocator_offset());
        }
        else {
            // FIXME: return an error
            terminate("No allocation is defined for this object");
        }
    }

    PackedAllocator* allocator_or_null()
    {
        if (allocator_offset() > 0)
        {
            uint8_t* my_ptr = ptr_cast<uint8_t>(this);
            return ptr_cast<PackedAllocator>(my_ptr - allocator_offset());
        }
        else {
            return nullptr;
        }
    }

    const PackedAllocator* allocator() const
    {
        if (allocator_offset() > 0)
        {
            const uint8_t* my_ptr = ptr_cast<const uint8_t>(this);
            return ptr_cast<const PackedAllocator>(my_ptr - allocator_offset());
        }
        else {
            // FIXME: return an error here
            terminate("No allocation is defined for this object");
        }
    }

    const PackedAllocator* allocator_or_null() const
    {
        if (allocator_offset() > 0)
        {
            const uint8_t* my_ptr = ptr_cast<const uint8_t>(this);
            return ptr_cast<const PackedAllocator>(my_ptr - allocator_offset());
        }
        else {
            return nullptr;
        }
    }

    static constexpr size_t round_up_bytes(size_t value)
    {
        return (value / AlignmentBlock + (value % AlignmentBlock ? 1 : 0)) * AlignmentBlock;
    }

    static constexpr size_t round_down_bytes(size_t value)
    {
        return (value / AlignmentBlock) * AlignmentBlock;
    }

    static constexpr size_t round_up_bits(size_t bits)
    {
        return round_up_bytes(round_up_bits_to_bytes(bits));
    }

    static constexpr size_t round_down_bits(size_t bits)
    {
        return round_down_bytes(round_down_bits_to_bytes(bits));
    }

    static constexpr size_t round_up_bits_to_bytes(size_t bits)
    {
        return bits / 8 + (bits % 8 > 0);
    }

    static constexpr size_t round_down_bits_to_bytes(size_t bits)
    {
        return bits / 8 + (bits % 8 > 0);
    }

    static constexpr size_t div_up(size_t value, size_t divider)  {
        return ::memoria::div_up(value, divider);
    }

    template <typename SerializationData>
    void serialize(SerializationData& buf) const
    {
        FieldFactory<psize_t>::serialize(buf, allocator_offset_);
    }

    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        FieldFactory<psize_t>::deserialize(buf, allocator_offset_);
    }
};



struct AllocationBlock {
    size_t size_;
    size_t offset_;
    uint8_t* ptr_;

    constexpr AllocationBlock(size_t size, size_t offset, uint8_t* ptr) :
        size_(size), offset_(offset), ptr_(ptr)
    {}

    constexpr AllocationBlock() :
        size_{}, offset_{}, ptr_{nullptr}
    {}

    size_t size() const    {return size_;}
    size_t offset() const  {return offset_;}
    uint8_t* ptr() const    {return ptr_;}
    bool is_empty() const   {return size_ == 0;}

    template <typename T>
    const T* cast() const  {
        return ptr_cast<const T>(ptr_);
    }

    template <typename T>
    T* cast()  {
        return ptr_cast<T>(ptr_);
    }

    operator bool() const  {
        return ptr_ != nullptr;
    }
};


struct AllocationBlockConst {
    size_t size_;
    size_t offset_;
    const uint8_t* ptr_;

    constexpr AllocationBlockConst(size_t size, size_t offset, const uint8_t* ptr) :
        size_(size), offset_(offset), ptr_(ptr)
    {}

    size_t size() const    {return size_;}
    size_t offset() const  {return offset_;}
    const uint8_t* ptr() const  {return ptr_;}

    operator bool() const  {return true;}

    template <typename T>
    const T* cast() const  {
        return ptr_cast<const T>(ptr_);
    }
};


enum class MMA_NODISCARD PkdUpdateStatus: bool {
    FAILURE, SUCCESS
};

class PackedAllocatorUpdateState {
    int64_t allocated_;
    int64_t available_;

    friend class PackedAllocator;

public:
    PackedAllocatorUpdateState(int64_t available = 0):
        allocated_(), available_(available)
    {}

    int64_t allocated() const {return allocated_;}
    int64_t available() const {return available_;}

    bool is_space_available() const {
        return allocated_ <= available_;
    }

    PkdUpdateStatus inc_allocated(int64_t existing_size, int64_t new_size) {
        allocated_ += new_size - existing_size;
        return is_space_available() ? PkdUpdateStatus::SUCCESS : PkdUpdateStatus::FAILURE;
    }
};


template <typename PkdStructSO>
class PkdStructUpdateBase {
    PackedAllocatorUpdateState* allocator_state_;
public:
    virtual ~PkdStructUpdateBase() noexcept = default;

    virtual void apply(PkdStructSO&) {};

    PackedAllocatorUpdateState* allocator_state() {return allocator_state_;}
    const PackedAllocatorUpdateState* allocator_state() const {return allocator_state_;}

    void set_allocator_state(PackedAllocatorUpdateState* state) {allocator_state_ = state;}
};


template <typename PkdStructSO>
struct PkdStructUpdateStepBase {

public:
    virtual ~PkdStructUpdateStepBase() noexcept = default;
    virtual void apply(PkdStructSO&) {};
};




template <typename PkdStructSO>
class PkdStructMultistepUpdate: public PkdStructUpdateBase<PkdStructSO> {
public:
    using UpdateT = PkdStructUpdateStepBase<PkdStructSO>;
private:

    std::vector<std::unique_ptr<UpdateT>> steps_;
public:
    PkdStructMultistepUpdate() {}

    void apply(PkdStructSO& so) {
        for (auto& update: steps_) {
            update->apply(so);
        }
    }

    template <typename T>
    T* make_new() {
        steps_.push_back(std::make_unique<T>());
        return static_cast<T*>(steps_[steps_.size() - 1].get());
    }
};


template <typename PkdStructSO>
class PkdStructUpdate: public PkdStructUpdateBase<PkdStructSO> {
public:
    using UpdateT = PkdStructUpdateStepBase<PkdStructSO>;
private:
    template <typename T>
    friend T* make_new(PkdStructUpdate<PkdStructSO>&);

    std::unique_ptr<UpdateT> step_;
public:
    PkdStructUpdate() {}

    operator bool() const {
        return step_;
    }

    template <typename T>
    T* step() {
        return static_cast<T*>(step_.get());
    }

    template <typename T>
    const T* step() const {
        return static_cast<const T*>(step_.get());
    }

    template <typename T>
    T* make_new() {
        step_ = std::make_unique<T>();
        return step<T>();
    }
};

template <typename PkdStructSO>
struct PkdStructEmptyUpdate: PkdStructUpdateBase<PkdStructSO>  {
};


template <typename PkdStructSO>
struct PkdStructNoOpUpdate {
    void set_allocator_state(PackedAllocatorUpdateState*) {}
};


#define MMA_MAKE_UPDATE_STATE_METHOD \
    std::pair<UpdateState, PackedAllocatorUpdateState> make_update_state() {\
        std::pair<UpdateState, PackedAllocatorUpdateState> state;           \
        state.second = this->data()->make_allocator_update_state();         \
        state.first.set_allocator_state(&state.second);                     \
        return state;                                                       \
    }                                                                       \
    UpdateState make_update_state(PackedAllocatorUpdateState* pa_state) {   \
        UpdateState state;                                                  \
        state.set_allocator_state(pa_state);                                \
        return state;                                                       \
    }




static inline bool is_success(PkdUpdateStatus status) {
    return status == PkdUpdateStatus::SUCCESS;
}

static inline void assert_success(PkdUpdateStatus status) {
    if (!is_success(status)) {
        MEMORIA_MAKE_GENERIC_ERROR("Packed update failed for the operation").do_throw();
    }
}

static inline void assert_success(bool status) {
    if (!status) {
        MEMORIA_MAKE_GENERIC_ERROR("Packed update failed for the operation").do_throw();
    }
}


}
