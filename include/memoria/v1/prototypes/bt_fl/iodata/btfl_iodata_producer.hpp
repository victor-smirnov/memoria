
// Copyright 2016 Victor Smirnov
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

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/prototypes/bt_fl/btfl_tools.hpp>
#include <memoria/v1/core/container/container.hpp>

#include <memoria/v1/prototypes/bt_fl/io/btfl_input.hpp>
#include <memoria/v1/prototypes/bt_fl/iodata/btfl_iodata_decl.hpp>

#include <algorithm>

namespace memoria {
namespace v1 {
namespace btfl {





enum class PopulateStatus {OK, IOBUFFER};

class PopulateState {
    PopulateStatus status_;
    int32_t entries_;
public:
    PopulateState(PopulateStatus status, int32_t entries): status_(status), entries_(entries) {}

    PopulateStatus status() const {return status_;}
    int32_t entries() const {return entries_;}
};




template <typename BTFLData, int32_t DataStreams, int32_t StartLevel = 0, typename IOBufferT = DefaultIOBuffer> class BTFLDataIOBufferProducerHelper;


template <int32_t DataStreams, int32_t StartLevel, typename IOBufferT, typename K, typename V, template <typename...> class Container, typename... Args>
class BTFLDataIOBufferProducerHelper<Container<std::tuple<K, V>, Args...>, DataStreams, StartLevel, IOBufferT> {
public:
    using BTFLDataT = Container<std::tuple<K, V>, Args...>;

protected:
    using NextBTFLIOBufferProducer = BTFLDataIOBufferProducerHelper<V, DataStreams, StartLevel + 1, IOBufferT>;
    using DataIterator             = typename BTFLDataT::const_iterator;

    bool value_finished_ = true;

    DataIterator start_;
    DataIterator end_;

    NextBTFLIOBufferProducer next_producer_;

public:
    BTFLDataIOBufferProducerHelper(const DataIterator& start, const DataIterator& end):
        start_(start),
        end_(end)
    {}

    BTFLDataIOBufferProducerHelper(const BTFLDataT& data):
        start_(data.begin()),
        end_(data.end())
    {}

    BTFLDataIOBufferProducerHelper() {}

    PopulateState populate(IOBufferT& buffer)
    {
      int32_t entries = 0;

      if (!value_finished_)
      {
          PopulateState state = next_producer_.populate(buffer);
          entries += state.entries();

          if (state.status() == PopulateStatus::OK)
          {
              value_finished_ = true;
              start_++;
          }
          else // IOBUFFER
          {
              return PopulateState(PopulateStatus::IOBUFFER, entries);
          }
      }


      while (start_ != end_)
      {
          if (buffer.template putSymbolsRun<DataStreams>(StartLevel, 1))
          {
              if (IOBufferAdapter<K>::put(buffer, std::get<0>(*start_)))
              {
                  entries += 2;

                  next_producer_ = NextBTFLIOBufferProducer(std::get<1>(*start_));

                  auto state = next_producer_.populate(buffer);
                  entries += state.entries();

                  if (state.status() == PopulateStatus::OK)
                  {
                      start_++;
                  }
                  else {
                      value_finished_ = false;
                      return PopulateState(PopulateStatus::IOBUFFER, entries);
                  }
              }
              else {
                  return PopulateState(PopulateStatus::IOBUFFER, entries);
              }
          }
          else {
              return PopulateState(PopulateStatus::IOBUFFER, entries);
          }
      }

      return PopulateState(PopulateStatus::OK, entries);
  }
};




template <int32_t DataStreams, int32_t StartLevel, typename IOBufferT, typename V, template <typename...> class Container, typename... Args>
class BTFLDataIOBufferProducerHelper<Container<V, Args...>, DataStreams, StartLevel, IOBufferT> {
public:
    using BTFLDataT = Container<V, Args...>;

protected:
    using DataIterator               = typename BTFLDataT::const_iterator;

    DataIterator start_;
    DataIterator end_;

public:
    BTFLDataIOBufferProducerHelper(const DataIterator& start, const DataIterator& end):
        start_(start),
        end_(end)
    {}

    BTFLDataIOBufferProducerHelper(const BTFLDataT& data):
        start_(data.begin()),
        end_(data.end())
    {
    }

    BTFLDataIOBufferProducerHelper() {}


    PopulateState populate(IOBufferT& buffer)
    {
      int32_t entries = 0;

      while (start_ != end_)
      {
          int32_t block_size = 256;
          size_t run_pos = buffer.pos();

          if (buffer.template putSymbolsRun<DataStreams>(StartLevel, block_size))
          {
              entries++;

              int32_t c;
              for (c = 0; c < block_size && start_ != end_; c++, start_++, entries++)
              {
                  if (!IOBufferAdapter<V>::put(buffer, *start_))
                  {
                      if (c > 0)
                      {
                          buffer.template updateSymbolsRun<DataStreams>(run_pos, StartLevel, c);
                          return PopulateState(PopulateStatus::IOBUFFER, entries);
                      }
                      else {
                          // do not "write" symbols run descriptor in this case
                          return PopulateState(PopulateStatus::IOBUFFER, entries - 1);
                      }
                  }
              }

              if (c < block_size) {
                  buffer.template updateSymbolsRun<DataStreams>(run_pos, StartLevel, c);
              }
          }
          else {
              return PopulateState(PopulateStatus::IOBUFFER, entries);
          }
      }

      return PopulateState(PopulateStatus::OK, entries);
  }
};



template <typename BTFLData, int32_t DataStreams, int32_t StartLevel = 0, typename IOBufferT = DefaultIOBuffer>
class BTFLDataIOBufferProducer: public bt::BufferProducer<IOBufferT> {

    using Helper = BTFLDataIOBufferProducerHelper<BTFLData, DataStreams, StartLevel, IOBufferT>;

    IOBufferT io_buffer_;
    Helper producer_helper_;

    using DataIterator = typename BTFLData::const_iterator;

public:

    BTFLDataIOBufferProducer(size_t capacity = 65536): io_buffer_(capacity) {}

    BTFLDataIOBufferProducer(const BTFLData& data, size_t capacity = 65536):
        io_buffer_(capacity),
        producer_helper_(data)
  {}

    BTFLDataIOBufferProducer(const DataIterator& start, const DataIterator& end, size_t capacity = 65536):
        io_buffer_(capacity),
        producer_helper_(start, end)
  {}

    void init(const BTFLData& data)
    {
        producer_helper_ = Helper(data);
    }

    void init(const DataIterator& start, const DataIterator& end)
    {
        producer_helper_ = Helper(start, end);
    }

    void clear() {
    }

    virtual IOBufferT& buffer() {return io_buffer_;}

    virtual int32_t populate(IOBufferT& buffer)
    {
        auto state = producer_helper_.populate(buffer);

        if (state.status() == PopulateStatus::IOBUFFER)
        {
            return state.entries();
        }
        else {
            return -state.entries();
        }
    }
};









}
}}
