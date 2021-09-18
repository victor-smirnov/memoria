
// Copyright 2021 Victor Smirnov
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
#include <memoria/core/strings/u8_string.hpp>
#include <memoria/core/tools/result.hpp>

#include <memory>
#include <type_traits>
#include <unordered_map>

namespace memoria {

struct IAnyID {
    virtual ~IAnyID() noexcept {}

    virtual const std::type_info& type_info() const noexcept = 0;

    virtual bool equals(const IAnyID& other) const noexcept = 0;
    virtual size_t hash_code() const noexcept = 0;
    virtual bool less(const IAnyID& other) const noexcept = 0;

    virtual U8String to_u8() const = 0;
    virtual bool to_bool() const noexcept = 0;

    virtual void unwrap_to(void* target) const noexcept = 0;
};

class AnyID {
    std::unique_ptr<IAnyID> id_;

    template <typename T>
    friend T cast_to(const AnyID&);

public:
    AnyID() {}
    AnyID(AnyID&& other) noexcept:
        id_(std::move(other.id_))
    {}

    AnyID(std::unique_ptr<IAnyID>&& id) noexcept :
        id_(std::move(id))
    {}

    AnyID& operator=(AnyID&& other) noexcept
    {
        id_ = std::move(other.id_);
        return *this;
    }

    bool operator==(const AnyID& other) const noexcept
    {
        if (id_ && other.id_ && id_->type_info() == other.id_->type_info()) {
            return id_->equals(*other.id_.get());
        }
        else {
            return false;
        }
    }

    U8String to_u8() const {
        return id_ ? id_->to_u8() : "<Empty AnyID>";
    }

    operator bool() const noexcept {
        return !id_ || id_->to_bool();
    }

    bool is_holder_empty() const noexcept {
        return !id_;
    }

    const std::type_info& type_info() const noexcept {
        return id_ ? id_->type_info() : typeid(AnyID);
    }

    bool operator<(const AnyID& other) const noexcept
    {
        if (id_ && other.id_ && id_->type_info() == other.id_->type_info()) {
            return id_->less(*other.id_.get());
        }
        else {
            return false;
        }
    }

    size_t hash_code() const noexcept {
        return id_? id_->hash_code() : 0;
    }

    template<typename T>
    static AnyID wrap(const T& id);
};

template <typename T>
T cast_to(const AnyID& id)
{
    if (id.id_)
    {
        if (id.id_->type_info() == typeid(T)) {
            T val;
            id.id_->unwrap_to(&val);
            return val;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Can't cast value of type '{}' to type '{}'", id.id_->type_info().name(), typeid(T).name()).do_throw();
        }
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("AnyID holder is empty").do_throw();
    }
}

template <typename ID>
class DefaultAnyIDImpl: public IAnyID {
    static_assert (std::is_trivially_copyable<ID>::value, "");
    ID id_;
public:
    DefaultAnyIDImpl(const ID& id) noexcept:
        id_(id)
    {}

    const std::type_info& type_info() const noexcept {
        return typeid(ID);
    }

    bool equals(const IAnyID& other) const noexcept {
        return id_ == cast(other).id_;
    }

    size_t hash_code() const noexcept {
        return std::hash<ID>()(id_);
    }

    bool less(const IAnyID& other) const noexcept {
        return id_ < cast(other).id_;
    }

    U8String to_u8() const {
        return id_.to_u8();
    }

    bool to_bool() const noexcept {
        return id_;
    }

    void unwrap_to(void* target) const noexcept {
        std::memcpy(target, &id_, sizeof(id_));
    }

protected:
    // It's guaranteed that other has correct type
    const DefaultAnyIDImpl<ID>& cast(const IAnyID& other) const noexcept {
        return *static_cast<const DefaultAnyIDImpl<ID>*>(&other);
    }
};

template<typename T>
inline AnyID AnyID::wrap(const T& id) {
    return AnyID{std::make_unique<DefaultAnyIDImpl<T>>(id)};
}

}
