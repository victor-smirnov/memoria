// Copyright 2019 Victor Smirnov
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

#include <memoria/v1/api/datatypes/type_signature.hpp>

#include <memoria/v1/core/memory/smart_ptrs.hpp>
#include <memoria/v1/core/tools/type_name.hpp>

#include <type_traits>

namespace memoria {
namespace v1 {

class DataType {
    SharedPtr<TypeSignature> signature_;

protected:
    DataType(SharedPtr<TypeSignature> signature):
        signature_(signature)
    {}

public:

    SharedPtr<TypeSignature> signature() const {
        return signature_;
    }

    bool equals(const Type& other) const {
        return signature_->equals(*other.signature_.get());
    }

    const U8String& name() const {
        return signature_->name();
    }
};

class FixedSizeType: public DataType {
    size_t size_;
protected:
    FixedSizeType(SharedPtr<TypeSignature> signature, size_t size):
        DataType(signature_), size_(size)
    {}
public:
    size_t size() const {
        return size_;
    }
};

class VariableSizeType: public DataType {
protected:
    VariableSizeType(SharedPtr<TypeSignature> signature):
        DataType(signature_)
    {}
public:
};


template <typename T>
class PrimitiveType: public FixedSizeType {
    static_assert(
        std::is_arithmetic<T>::value,
        "Provided type must be an arithmetic type"
    );
public:
    PrimitiveType():
        FixedSizeType(make_type_signature<T>(), sizeof(T))
    {}
};


}}
