
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

#include <memoria/api/common/ctr_api_btss.hpp>

#include <memoria/api/common/iobuffer_adatpters.hpp>

#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/datatypes/encoding_traits.hpp>
#include <memoria/core/datatypes/io_vector_traits.hpp>

#include <memoria/core/iovector/io_vector.hpp>

namespace memoria {

namespace _ {

template <typename ValueT, bool FixedSizeValue>
struct VectorValues;



template <typename ValueT>
struct VectorValues<ValueT, false>
{

    ArenaBuffer<ValueT> values_;

    Span<const ValueT> values() const {return values_.span();}

    void prepare(size_t size) {
        values_.clear();
        values_.ensure(size);
    }
};


template <typename ValueT>
struct VectorValues<ValueT, true>
{
    Span<const ValueT> values_;

    const Span<const ValueT>& values() const {return values_;}

    void prepare(size_t size) {}
};

}


template <typename Types, typename Profile>
class VectorScanner {
public:
    using ValueView = DTTViewType<typename Types::Value>;

private:

    static constexpr bool DirectValues = DTTIs1DFixedSize<typename Types::Value>;

    using IOVSchema = Linearize<typename Types::IOVSchema>;

    bool finished_{false};

    _::VectorValues<ValueView, DirectValues> entries_;

    CtrSharedPtr<BTSSIterator<Profile>> btss_iterator_;

public:
    VectorScanner(CtrSharedPtr<BTSSIterator<Profile>> iterator):
        btss_iterator_(iterator)
    {
        populate();
    }

    Span<const ValueView> values() const {return entries_.values();}

    bool is_end() const {
        return finished_ || btss_iterator_->is_end();
    }

    bool next_leaf()
    {
        bool available = btss_iterator_->next_leaf();
        if (available) {
            populate();
        }
        else {
            finished_ = true;
        }

        return available;
    }

private:
    void populate()
    {
        const io::IOVector& iovector = btss_iterator_->iovector_view();

        int32_t size = iovector.symbol_sequence().size();
        if (size > 0)
        {
            entries_.prepare(size);

            int32_t start = btss_iterator_->iovector_pos();

            IOSubstreamAdapter<Select<0, IOVSchema>>::read_to(
                iovector.substream(0),
                0, // Column
                start,
                size - start,
                entries_.values_
            );
        }
    }
};

}
