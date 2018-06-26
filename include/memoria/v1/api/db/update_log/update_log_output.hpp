
// Copyright 2017 Victor Smirnov
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

#include <memoria/v1/api/common/ctr_api_btfl.hpp>
#include <memoria/v1/api/common/ctr_input_btss.hpp>

#include <memoria/v1/api/db/update_log/update_log_input.hpp>

#include <memoria/v1/core/tools/uuid.hpp>

#include <memoria/v1/core/tools/object_pool.hpp>



#include <memory>
#include <tuple>

namespace memoria {
namespace v1 {


struct UpdateLogFindResult {
    UAcc128T prefix;
    int64_t pos;
    int64_t size;

    bool is_found() const {return pos >= 0 && pos < size;}
};


namespace update_log {

template <typename Iterator, typename CtrSizeT>
class CommandsDataIterator {
    Iterator iterator_;

    int32_t entry_{};
    int32_t n_entries_{};

    bool empty_{};

    PoolUniquePtr<DefaultIOBuffer> buffer_{};
    std::unique_ptr<IBTFLPopulateWalker<DefaultIOBuffer>> walker_{};

public:
    CommandsDataIterator(): iterator_{nullptr} {}
    CommandsDataIterator(CommandsDataIterator&& other) = default;

    CommandsDataIterator(Iterator ii):
        iterator_(ii)
    {
        prepare();
    }

    void dump() {
        if (iterator_.ptr()) {
            iterator_.dump();
        }
    }

    bool has_next() {return entry_ < n_entries_ || ((!empty_) && prefetch()) ;}

    CtrSizeT seek(CtrSizeT pos);
    CtrSizeT remove(CtrSizeT length);

    CtrSizeT pos();
    CtrSizeT size();

    size_t fetch(uint8_t* mem, size_t size)
    {
        size_t cnt{};
        while (cnt < size && has_next())
        {
            size_t len = fetch_buffer(mem, size - cnt);
            mem += len;
            cnt += len;
        }

        return cnt;
    }

    template <typename OutputIterator>
    size_t fetch(OutputIterator& ii, size_t max = std::numeric_limits<size_t>::max())
    {
        size_t cnt{};
        while(cnt < max && has_next())
        {
            *ii = next();
            ++ii;

            cnt++;
        }

        return cnt;
    }

    std::vector<uint8_t> as_vector()
    {
        std::vector<uint8_t> vv;

        while (has_next())
        {
            vv.push_back(next());
        }

        return vv;
    }

    uint8_t next()
    {
        if (MMA1_LIKELY(has_next()))
        {
            entry_++;
            return IOBufferAdapter<uint8_t>::get(*buffer_.get());
        }

        MMA1_THROW(RuntimeException()) << WhatCInfo("No such element");
    }


protected:
    bool prefetch();
    void prepare();

    size_t fetch_buffer(uint8_t* mem, size_t size)
    {
        size_t available = n_entries_ - entry_;
        size_t to_read = size <= available ? size : available;

        buffer_->get(mem, to_read);

        n_entries_ += to_read;

        return to_read;
    }
};



template <typename Iterator, typename CtrSizeT>
class ContainerNameIterator {

    Iterator iterator_;

    UAcc192T prefix_{};

    CtrSizeT pos_{};
    CtrSizeT size_{};

public:
    using CommandsDataIteratorT = CommandsDataIterator<Iterator, CtrSizeT>;

    ContainerNameIterator(): iterator_{nullptr} {}
    ContainerNameIterator(Iterator ii, const UAcc192T& prefix, CtrSizeT pos, CtrSizeT size):
        iterator_(ii), prefix_{prefix}, pos_(pos), size_(size)
    {

    }

    bool has_next() {return pos_ < size_ ;}

    void dump() {
        if (iterator_.ptr()) {
            iterator_.dump();
        }
    }

    UUID next();

    size_t fetch(UUID* mem, size_t size)
    {
        size_t cnt{};

        while (cnt < size && has_next())
        {
            *mem = next();
            mem++;
            cnt++;
        }

        return cnt;
    }

    template <typename OutputIterator>
    size_t fetch(OutputIterator& ii, size_t max = std::numeric_limits<size_t>::max())
    {
        size_t cnt{};
        while(cnt < max && has_next())
        {
            *ii = next();
            ++ii;
        }
    }

    std::vector<UUID> as_vector()
    {
        std::vector<UUID> vv;

        while (has_next()){
            vv.push_back(next());
        }

        return vv;
    }

    CommandsDataIteratorT commands();
};





template <typename Iterator, typename CtrSizeT>
class SnapshotIDIterator {
    Iterator iterator_;

    CtrSizeT pos_{};
    CtrSizeT size_{};


public:
    using ContainerNameIteratorT = ContainerNameIterator<Iterator, CtrSizeT>;

    SnapshotIDIterator(): iterator_{nullptr} {}
    SnapshotIDIterator(SnapshotIDIterator&& other) = default;

    SnapshotIDIterator(Iterator ii, CtrSizeT pos, CtrSizeT size):
        iterator_{ii}, pos_(pos), size_(size)
    {}

    void dump() {
        if (iterator_.ptr()) {
            iterator_.dump();
        }
    }

    bool has_next() {return pos_ < size_;}

    UUID next();

    size_t fetch(UUID* mem, size_t size)
    {
        size_t cnt{};

        while (cnt < size && has_next())
        {
            *mem = next();
            mem++;
            cnt++;
        }

        return cnt;
    }

    template <typename OutputIterator>
    size_t fetch(OutputIterator& ii, size_t max = std::numeric_limits<size_t>::max())
    {
        size_t cnt{};
        while(cnt < max && has_next())
        {
            *ii = next();
            ++ii;
        }
    }

    std::vector<UUID> as_vector()
    {
        std::vector<UUID> vv;

        while (has_next()){
            vv.push_back(next());
        }

        return vv;
    }

    ContainerNameIteratorT containers();
    CtrSizeT containers_size();
};





//template <typename Iterator, typename CtrSizeT>
//class SnapshotIDIterator {
//    Iterator iterator_;

//    int32_t entry_{};
//    int32_t n_entries_{};

//    bool empty_{};

//    PoolUniquePtr<DefaultIOBuffer> buffer_;

//    std::unique_ptr<IBTFLPopulateWalker<DefaultIOBuffer>> walker_{};

//public:
//    using ContainerNameIteratorT = ContainerNameIterator<Iterator, CtrSizeT>;

//    SnapshotIDIterator(): iterator_{nullptr}, empty_{true} {}
//    SnapshotIDIterator(SnapshotIDIterator&& other) = default;

//    SnapshotIDIterator(Iterator ii): iterator_{ii}
//    {
//        prepare();
//    }

//    bool has_next() {return entry_ < n_entries_ || ((!empty_) && prefetch()) ;}

//    UUID next()
//    {
//        if (MMA1_LIKELY(has_next()))
//        {
//            entry_++;
//            return IOBufferAdapter<UUID>::get(*buffer_.get());
//        }

//        MMA1_THROW(RuntimeException()) << WhatCInfo("No such element");
//    }

//    size_t fetch(UUID* mem, size_t size);

//    template <typename OutputIterator>
//    size_t fetch(OutputIterator& ii, size_t max = std::numeric_limits<size_t>::max())
//    {
//        size_t cnt{};
//        while(cnt < max && has_next())
//        {
//            *ii = next();
//            ++ii;
//        }
//    }

//    std::vector<UUID> as_vector()
//    {
//        std::vector<UUID> vv;

//        while (has_next()){
//            vv.push_back(next());
//        }

//        return vv;
//    }

//    ContainerNameIteratorT containers();

//protected:
//    bool prefetch();
//    void prepare();
//};


}

}}
