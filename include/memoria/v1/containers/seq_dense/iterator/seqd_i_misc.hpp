
// Copyright 2011 Victor Smirnov
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

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/tools/dump.hpp>

#include <memoria/v1/containers/seq_dense/seqd_names.hpp>
#include <memoria/v1/containers/seq_dense/seqd_tools.hpp>
#include <memoria/v1/core/container/iterator.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <memoria/v1/prototypes/bt/bt_macros.hpp>

#include <memoria/v1/core/packed/array/packed_fse_bitmap.hpp>

namespace memoria {
namespace v1 {


MEMORIA_V1_ITERATOR_PART_BEGIN(v1::seq_dense::IterMiscName)
public:
    typedef Ctr<typename Types::CtrTypes>                                       Container;


    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Container::BranchNodeEntry                                 BranchNodeEntry;
    typedef typename Container::Iterator                                        Iterator;

    typedef typename Container::Types::Pages::LeafDispatcher                    LeafDispatcher;
    typedef typename Container::Position                                        Position;

    using SymbolsSubstreamPath = typename Container::Types::SymbolsSubstreamPath;

    using typename Base::CtrSizeT;

    template <int32_t Stream>
    using InputTupleAdapter = typename Container::Types::template InputTupleAdapter<Stream>;

    static const int32_t BitsPerSymbol  = Container::Types::BitsPerSymbol;
    static const int32_t Symbols        = Container::Types::Symbols;


    int32_t symbol() const
    {
        auto& self  = this->self();
        return std::get<0>(self.ctr().template read_leaf_entry<IntList<1>>(self.leaf(), self.idx()));
    }


    void setSymbol(int32_t symbol)
    {
        auto& self  = this->self();

        self.ctr().template update_stream_entry<0, IntList<1>>(self, std::make_tuple(symbol));
    }

    struct InsertSymbolFn {

        int32_t symbol_;

        CtrSizeT one_ = 1;

        InsertSymbolFn(int32_t symbol): symbol_(symbol) {}

        const auto& get(StreamTag<0>, StreamTag<0>, int32_t block) const {
            return one_;
        }

        const auto& get(StreamTag<0>, StreamTag<1>, int32_t block) const {
            return symbol_;
        }
    };


    void insert_symbol(int32_t symbol)
    {
        MEMORIA_V1_ASSERT(symbol, <, (int32_t)Symbols);

        auto& self  = this->self();
        auto& ctr   = self.ctr();

        ctr.insert_entry(
                self,
                InsertSymbolFn(symbol)
        );

        self.skipFw(1);
    }


//    template <typename T>
//    struct ReadWalker {
//        T& data_;
//        CtrSizeT processed_ = 0;
//        const CtrSizeT max_;
//
//        ReadWalker(T& data): data_(data), max_(data.size()) {}
//
//        template <typename NodeTypes>
//        int32_t treeNode(const LeafNode<NodeTypes>* leaf, int32_t start)
//        {
//            return std::get<0>(leaf->template processSubstreams<IntList<0, 0>>(*this, start));
//        }
//
//        template <typename StreamObj>
//        int32_t stream(const StreamObj* obj, int32_t start)
//        {
//            if (obj != nullptr)
//            {
//                int32_t size        = obj->size();
//                int32_t remainder   = size - start;
//
//                int32_t to_read = (processed_ + remainder < max_) ? remainder : (max_ - processed_);
//
//                obj->read(&data_, start, processed_, to_read);
//
//                return to_read;
//            }
//            else {
//                return 0;
//            }
//        }
//
//        void start_leaf() {}
//
//        void end_leaf(int32_t skip) {
//            processed_ += skip;
//        }
//
//        CtrSizeT result() const {
//            return processed_;
//        }
//
//        bool stop() const {
//            return processed_ >= max_;
//        }
//    };



//    int64_t read(SymbolsBuffer<BitsPerSymbol>& data)
//    {
//        auto& self = this->self();
//        ReadWalker<SymbolsBuffer<BitsPerSymbol>> target(data);
//
//        return self.ctr().readStream2(self, target);
//    }
//
//
//    void insert_symbols(SymbolsBuffer<BitsPerSymbol>& data)
//    {
//        auto& self = this->self();
//        auto& model = self.ctr();
//
//        auto& leaf = self.leaf();
//
//        seq_dense::SymbolsInputBufferProvider<BitsPerSymbol> provider(data);
//
//        auto result = model.insertBuffers(leaf, self.idx(), provider);
//
//        self.leaf() = result.leaf();
//        self.idx() = result.position();
//
//        self.refresh();
//
//        model.markCtrUpdated();
//    }



    template <typename Seq>
    class ReadAdaptor {
        const Seq& seq_;
        int32_t pos_ = 0;

    public:
        ReadAdaptor(const Seq& seq): seq_(seq) {}

        template <typename V>
        void process(StreamTag<0>, StreamTag<1>, int32_t start, int32_t end, V&& value)
        {
            for (int32_t c = start; c < end; c++)
            {
                auto sym1 = seq_.symbol(pos_ + c - start);
                auto sym2 = value.symbol(c);

                if (sym1 != sym2) {
                    cout << "Mismatch! " << sym1 << " " << sym2 << " " << (pos_ + c - start) << endl;
                }
            }

            pos_ += (end - start);
        }
    };


//    template <typename Seq>
//    auto read(Seq&& seq)
//    {
//        auto& self = this->self();
//
//        ReadAdaptor<Seq> adaptor(&seq);
//
//        return self.ctr().template read_single_substream2<IntList<0, 1>>(self, length, adaptor);
//    }

    template <typename Seq>
    auto compare(Seq&& seq)
    {
        auto& self = this->self();

        ReadAdaptor<Seq> adaptor(seq);

        return self.ctr().template read_single_substream2<IntList<0, 1>>(self, seq.size(), adaptor);
    }


    void check(const char* source = nullptr) const
    {
        auto& self = this->self();

        auto tmp = self;

        tmp.refresh();

        if (self.cache() != tmp.cache())
        {
            throw TestException(
                    source != nullptr ? source : MA_SRC,
                    SBuf()<<"Iterator cache mismatch: having: "<<self.cache()<<", should be: "<<tmp.cache()
            );
        }
    }

    template <typename Provider>
    CtrSizeT bulk_insert(Provider&& provider)
    {
        auto& self = this->self();
        return self.ctr().insert(self, std::forward<Provider>(provider));
    }


MEMORIA_V1_ITERATOR_PART_END


#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(v1::seq_dense::IterMiscName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS




#undef M_TYPE
#undef M_PARAMS


}}
