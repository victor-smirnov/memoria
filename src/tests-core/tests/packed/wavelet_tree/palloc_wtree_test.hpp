
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

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/core/packed/wavelet_tree/packed_wavelet_tree.hpp>
#include <memoria/core/packed/sseq/packed_multisequence.hpp>
#include <memoria/core/packed/tools/packed_struct_ptrs.hpp>

#include <memory>
#include <map>

namespace memoria {


class PackedWaveletTreeTest: public TestTask {

    typedef TestTask                                                        Base;
    typedef PackedWaveletTreeTest                                           MyType;

    typedef PackedWaveletTreeTypes<>                                        Types;

    using WaveletTree    = PackedWaveletTree<Types>;
    using WaveletTreePtr = PkdStructSPtr<WaveletTree>;

    typedef typename WaveletTree::CardinalTree::LoudsTree                   LoudsTree;

    typedef pair<uint32_t, size_t>                                                 Pair;

    size_t alphabet_size_ = 10;

public:

    PackedWaveletTreeTest(): TestTask("WaveletTree")
    {
        size_ = 514;

        MEMORIA_ADD_TEST_PARAM(alphabet_size_);

        MEMORIA_ADD_TEST(testCreateTree);
        MEMORIA_ADD_TEST(testRemoveTree);
    }

    virtual ~PackedWaveletTreeTest() noexcept {}



    void traverseTreePaths(const LoudsTree* tree, function<void (const PackedLoudsNode&, size_t)> fn, size_t level = 0)
    {
        traverseTreePaths(tree, tree->root(), PackedLoudsNode(), fn, level);
    }

    void traverseTreePaths(
            const LoudsTree* tree,
            const PackedLoudsNode& node,
            const PackedLoudsNode& parent,
            function<void (const PackedLoudsNode&, size_t)> fn, size_t level = 0)
    {
        if (tree->isLeaf(node))
        {
            fn(parent, level - 1);
        }
        else
        {
            PackedLoudsNode child = tree->first_child(node);
            while (child != PackedLoudsNode() && !tree->isLeaf(child))
            {
                traverseTreePaths(tree, child, node, fn, level + 1);
                child = tree->right_sibling(child);
            }
        }
    }



    WaveletTreePtr createTree(size_t block_size = 128*1024)
    {
        return MakeSharedPackedStructByBlock<WaveletTree>(block_size);
    }

    vector<uint32_t> createRandomAlphabet(size_t size)
    {
        vector<uint32_t> text(size);

        for (auto& v: text)
        {
            v = getRandom();
        }

        return text;
    }

    vector<uint32_t> createRandomText(size_t size, const vector<uint32_t>& alphabet)
    {
        vector <uint32_t> text(size);

        for (auto& v: text)
        {
            v = alphabet[getRandom(alphabet.size())];
        }

        return text;
    }

    vector<Pair> getRankedSymbols(const vector<uint32_t>& text)
    {
        vector<Pair> result;

        std::map<uint32_t, size_t> ranks;

        for (uint32_t c = 0; c < text.size(); c++)
        {
            uint32_t letter = text[c];

            if (ranks.find(letter) == ranks.end())
            {
                ranks[letter] = 0;
            }

            ranks[letter]++;
        }

        for (auto pair: ranks)
        {
            result.push_back(pair);
        }

        std::sort(result.begin(), result.end(), [](const Pair& a, const Pair& b ) {
            return a.second > b.second;
        });

        return result;
    }

    size_t rank(const vector<uint32_t>& text, size_t idx, uint32_t symbol)
    {
        size_t sum = 0;

        for (size_t c = 0; c <= idx; c++)
        {
            sum += text[c] == symbol;
        }

        return sum;
    }

    size_t select(const vector<uint32_t>& text, size_t rank, uint32_t symbol)
    {
        for (uint32_t c = 0; c < text.size(); c++)
        {
            rank -= text[c] == symbol;

            if (rank == 0)
            {
                return c;
            }
        }

        return text.size();
    }

    void assertText(const WaveletTreePtr& tree, const vector<uint32_t>& text)
    {
        AssertEQ(MA_SRC, tree->size(), (size_t)text.size());

        for (uint32_t c = 0; c < text.size(); c++)
        {
            uint32_t value = tree->value(c);

            AssertEQ(MA_SRC, value, text[c], SBuf()<<c);
        }
    }



    void testCreateTree()
    {
        auto tree = createTree();

        tree->prepare();

        auto fn = [](const PackedLoudsNode& node, size_t level) {
            AssertEQ(MA_SRC, level, 3);
        };

        auto alphabet = createRandomAlphabet(alphabet_size_);

        auto text = createRandomText(size_, alphabet);

        for (uint32_t c = 0; c < text.size(); c++)
        {
            uint64_t path = text[c];

            out()<<"Insert at "<<c<<": "<<hex<<path<<dec<<std::endl;
            tree->insert(c, path);

            traverseTreePaths(tree->ctree()->tree(), fn);
        }

        assertText(tree, text);

        auto ranks = getRankedSymbols(text);

        for (auto& rnk: ranks)
        {
            uint32_t sym = rnk.first;

            for (uint32_t c = 0; c < text.size(); c++)
            {
                size_t rank1 = tree->rank(c, sym);
                size_t rank2 = rank(text, c, sym);

                AssertEQ(MA_SRC, rank1, rank2);
            }
        }

        for (auto& rnk: ranks)
        {
            uint32_t sym = rnk.first;
            size_t rank = rnk.second;

            for (size_t r = 1; r <= rank; r++)
            {
                size_t idx1 = tree->select(r, sym);
                size_t idx2 = select(text, r, sym);

                AssertEQ(MA_SRC, idx1, idx2);
            }
        }

        tree->dump(this->out());
    }

    void testRemoveTree()
    {
        auto tree = createTree();

        tree->prepare();

        out()<<"Process new tree"<<std::endl;
        testRemoveTree(tree);

        out()<<"Process used empty tree"<<std::endl;
        testRemoveTree(tree);
    }

    void testRemoveTree(WaveletTreePtr& tree)
    {
        auto fn = [](const PackedLoudsNode& node, size_t level) {
            AssertEQ(MA_SRC, level, 3);
        };

        auto alphabet = createRandomAlphabet(alphabet_size_);

        auto text = createRandomText(size_, alphabet);

        for (auto t: text) {
            out()<<hex<<t<<endl;
        }

        for (uint32_t c = 0; c < text.size(); c++)
        {
            uint64_t path = text[c];
            tree->insert(c, path);
        }

        traverseTreePaths(tree->ctree()->tree(), fn);

        tree->dump(out());

        while (tree->size() > 0)
        {
            size_t idx = getRandom(tree->size());

            out()<<"Remove: "<<idx<<" "<<hex<<tree->value(idx)<<dec<<std::endl;

            tree->remove(idx);

            text.erase(text.begin() + idx);

            assertText(tree, text);
        }

        tree->dump(out());
    }
};


}
