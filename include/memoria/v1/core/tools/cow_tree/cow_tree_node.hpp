
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/tools/optional.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/core/tools/md5.hpp>

#include <algorithm>

namespace memoria {
namespace cow       {
namespace tree      {

enum class NodeType {LEAF, BRANCH};

template <typename Key, Int NodeSize, Int NodeIndexSize>
class NodeBase {
public:
    class RootMetadata {
        BigInt size_ = 0;
    public:
        BigInt& size() {
            return size_;
        }
        const BigInt& size() const {
            return size_;
        }

        void add_size(BigInt delta) {
            size_ += delta;
        }
    };
private:

    template <typename, typename> friend class CoWTree;

    RootMetadata metadata_;

    NodeType node_type_;

    BigInt node_id_;
    BigInt txn_id_;
    Int size_ = 0;

protected:
    BigInt refs_ = 0;
private:

    static constexpr Int Indexes =  NodeSize / NodeIndexSize + ((NodeSize % NodeIndexSize == 0)?  0 : 1);

    Key index_[Indexes];
    Key keys_ [NodeSize];

public:
    NodeBase(BigInt txn_id, BigInt node_id, NodeType node_type):
        node_type_(node_type),
        node_id_(node_id),
        txn_id_(txn_id)
{}

    RootMetadata& metadata() {
        return metadata_;
    }

    const RootMetadata& metadata() const {
        return metadata_;
    }

    BigInt node_id() const {
        return node_id_;
    }

    NodeType node_type() const {
        return node_type_;
    }

    bool is_leaf() const {
        return node_type_ == NodeType::LEAF;
    }

    bool is_branch() const {
        return node_type_ == NodeType::BRANCH;
    }

    BigInt txn_id() const {
        return txn_id_;
    }

    Int size() const {
        return size_;
    };

    Int max_size() const {
        return NodeSize;
    }

    Int capacity() const {
        return max_size() - size();
    }

    bool should_merge() const {
        return size_ <= max_size() / 2;
    }

    const Key& max_key() const {
        return keys_[size_ - 1];
    }

    const Key& min_key() const {
        return keys_[0];
    }

    bool is_full() const {
        return size_ == max_size();
    }

    bool has_space() const {
        return size_ < max_size();
    }

    BigInt refs() const {
        return refs_;
    }

    BigInt ref() {
        return ++refs_;
    }

    BigInt unref() {
        return --refs_;
    }

    const Key& key(Int idx) const {
        return keys_[idx];
    }

    Key& key(Int idx) {
        return keys_[idx];
    }

//  Int find_key(const Key& key) const
//  {
//      int i1 = find_key1(key);
//      int i2 = find_key2(key);
//
//      if (i1 != i2)
//      {
//          i2 = find_key2(key);
//
//          cout<<"key="<<key<<" "<<i1<<" "<<i2<<endl;
//      }
//
//      return i1;
//  }

    Int find_key(const Key& key) const
    {
        Int last_idx = size_ / NodeIndexSize + (size_ % NodeIndexSize == 0 ? 0 : 1);

        Int idx = -1;
        for (Int c = 0; c < last_idx; c++)
        {
            if (key <= index_[c])
            {
                idx = c;
                break;
            }
        }

        if (idx >= 0)
        {
            idx *= NodeIndexSize;

            for (Int c = idx; c < idx + NodeIndexSize; c++)
            {
                if (key <= keys_[c])
                {
                    return c;
                }
            }
        }

        return size_;
    }

    Int find_key2(const Key& key) const
    {
        Int last_idx = size_ / NodeIndexSize + (size_ % NodeIndexSize == 0 ? 0 : 1);

        Int idx = 0;
        for (Int c = 0; c < last_idx; c++)
        {
            idx += (key > index_[c]);
        }

        if (idx < last_idx)
        {
            idx *= NodeIndexSize;

            Int max = (idx + NodeIndexSize) < size_ ? idx + NodeIndexSize : size_;

            for (Int c = idx; c < max; c++)
            {
                idx += (key > keys_[c]);
            }

            return idx;
        }

        return size_;
    }


    Int find_key3(const Key& key) const
    {
        return std::distance(keys_, std::lower_bound(keys_, keys_ + size_, key));
    }

    void reindex()
    {
        if (size_ >= 18) {
            int a = 0; a++;
        }

        for (auto& k: index_) k = Key();

        Int max = (size_ % NodeIndexSize == 0) ? size_ : (size_ - size_ % NodeIndexSize);

        for (Int idx = 0; idx < max; idx += NodeIndexSize)
        {
            index_[idx / NodeIndexSize] = keys_[idx + NodeIndexSize - 1];
        }

        Int last_element = size_ - 1;

        index_[last_element / NodeIndexSize] = keys_[last_element];
    }

    void insert_key(Int idx, const Key& key)
    {
        shift_right(keys_, idx, idx + 1, size_ - idx);
        size_ ++;

        keys_[idx] = key;

        reindex();
    }

    void remove_keys(Int start, Int end)
    {
        shift_left(keys_, end, start, size_ - end);

        for (Int c = size() - (end - start); c < size(); c++) {
            keys_[c] = Key();
        }

        size_ -= (end - start);
        reindex();
    }

    void split_keys_to(Int start, NodeBase* other)
    {
        copy_to(keys_, start, other->keys_, 0, size_ - start);

        for (Int c = start; c < size(); c++)
        {
            keys_[c] = Key();
        }

        other->size_ += size_ - start;
        this->size_  -= size_ - start;

        this->reindex();
        other->reindex();
    }

    void merge_keys_from(NodeBase* other)
    {
        copy_to(other->keys_, 0, keys_, size_, other->size_);

        this->size_  += other->size_;
        other->size_ = 0;

        this->reindex();
        other->reindex();
    }

    bool can_merge_with(const NodeBase* other) const {
        return size_ + other->size_ <= NodeSize;
    }

    void dump(std::ostream& out) const
    {
        out<<"NodeType: "<<node_type()<<endl;
        out<<"Size: "<<size_<<endl;
        out<<"NodeId: "<<node_id_<<endl;
        out<<"TxnId: "<<txn_id_<<endl;

        out<<"Index: "<<endl;

        for (Int c = 0; c < Indexes; c++)
        {
            out<<c<<": "<<index_[c]<<endl;
        }
    }

protected:
    template <typename T>
    void shift_right(T* data, Int from, Int to, Int size) const
    {
        CopyBuffer(data + from, data + to, size);
    }

    template <typename T>
    void shift_left(T* data, Int from, Int to, Int size) const
    {
        CopyBuffer(data + from, data + to, size);
    }

    template <typename T>
    void copy_to(const T* src, Int from, T* dst, Int to, Int size) const
    {
        CopyBuffer(src + from, dst + to, size);
    }

    void set_txn_id(BigInt txn_id) {
        this->txn_id_ = txn_id;
    }

    void set_node_id(BigInt id) {
        this->node_id_ = id;
    }

    void clear_refs() {
        refs_ = 0;
    }

    void set_refs(Int refs) {
        this->refs_ = refs;
    }

    void hash(MD5Hash& md5) const
    {
        md5.add((UInt)node_type_);
        md5.add_ubi(metadata_.size());
        md5.add_ubi(node_id_);
        md5.add_ubi(txn_id_);
        md5.add_ubi(size_);

        for (Int c = 0; c < size_; c++) {
            md5.add_ubi(keys_[c]);
        }
    }
};


template <typename Key, typename Data, Int NodeSize, Int NodeIndexSize>
class Node: public NodeBase<Key, NodeSize, NodeIndexSize> {
    using Base = NodeBase<Key, NodeSize, NodeIndexSize>;

protected:
    Data data_[NodeSize];

public:
    using MyType    = Node<Key, Data, NodeSize, NodeIndexSize>;
    using NodeBaseT = NodeBase<Key, NodeSize, NodeIndexSize>;


    Node(BigInt txn_id, BigInt node_id, NodeType node_type):
        NodeBase<Key, NodeSize, NodeIndexSize>(txn_id, node_id, node_type)
    {}

    const Data& data(Int idx) const {
        return data_[idx];
    }

    Data& data(Int idx) {
        return data_[idx];
    }

    void insert(Int idx, const Key& key, const Data& child)
    {
        this->shift_right(data_, idx, idx + 1, this->size() - idx);

        data_[idx] = child;

        this->insert_key(idx, key);
    }

    void remove(Int idx) {
        remove(idx, idx + 1);
    }

    void remove(Int start, Int end)
    {
        this->shift_left(data_, end, start, this->size() - end);

        for (Int c = this->size() - (end - start); c < this->size(); c++) {
            data_[c] = Data();
        }

        this->remove_keys(start, end);
    }

    void split_to(Int start, MyType* other)
    {
        this->copy_to(data_, start, other->data_, 0, this->size() - start);

        for (Int c = start; c < this->size(); c++)
        {
            data_[c] = Data();
        }

        this->split_keys_to(start, other);
    }

    void merge_from(MyType* other)
    {
        this->copy_to(other->data_, 0, data_, this->size(), other->size());
        this->merge_keys_from(other);
    }

    UBigInt hash() const
    {
        MD5Hash md5;

        md5.add_ubi((UBigInt)this);

        Base::hash(md5);

        for (Int c = 0; c < this->size(); c++) {
            md5.add_ubi((UBigInt)this->data(c));
        }

        return md5.result().hash64();
    }
};


template <typename Key, Int NodeSize, Int NodeIndexSize>
class BranchNode: public Node<Key, NodeBase<Key, NodeSize, NodeIndexSize>*, NodeSize, NodeIndexSize> {
    using Base = Node<Key, NodeBase<Key, NodeSize, NodeIndexSize>*, NodeSize, NodeIndexSize>;

public:
    using NodeBaseT = NodeBase<Key, NodeSize, NodeIndexSize>;

    BranchNode(BigInt txn_id, BigInt node_id):
        Base(txn_id, node_id, NodeType::BRANCH)
    {}

    NodeBaseT* find_child(const Key& key) const
    {
        Int idx = Base::find_key(key);

        if (idx == this->size())
        {
            idx = this->size() - 1;
        }

        return this->data_[idx];
    }

    Int find_child_node(const NodeBaseT* child) const
    {
        for (Int c = 0; c < this->size(); c++)
        {
            if (this->data_[c] == child)
            {
                return c;
            }
        }

        return -1;
    }

    NodeBaseT* first_child() {
        return this->data(0);
    }

    const NodeBaseT* first_child() const {
        return this->data(0);
    }

    NodeBaseT* last_child() {
        return this->data(this->size() - 1);
    }

    const NodeBaseT* last_child() const {
        return this->data(this->size() - 1);
    }

    void dump(std::ostream& out = std::cout) const
    {
        Base::dump(out);

        out<<"Data: "<<endl;

        for (Int c = 0; c < this->size(); c++)
        {
            auto node = this->data(c);
            out<<c<<": "<<this->key(c)<<" = "<<node<<" ("<<node->node_id()<<", "<<node->txn_id()<<")"<<endl;
        }

        cout<<endl;
    }


};

template <typename Key, typename Value, Int NodeSize, Int NodeIndexSize>
class LeafNode: public Node<Key, Value, NodeSize, NodeIndexSize> {
    using Base = Node<Key, Value, NodeSize, NodeIndexSize>;
public:
    LeafNode(BigInt txn_id, BigInt node_id):Base(txn_id, node_id, NodeType::LEAF) {}

    Int find(const Key& key) const
    {
        return this->find_key(key);
    }

    void dump(std::ostream& out = std::cout) const
    {
        Base::dump(out);

        out<<"Data: "<<endl;

        for (Int c = 0; c < this->size(); c++)
        {
            out<<c<<": "<<this->key(c)<<" = "<<this->data(c)<<endl;
        }

        cout<<endl;
    }


};


std::ostream& operator<<(std::ostream& out, NodeType node_type) {
    if (node_type == NodeType::LEAF) {
        out<<"LEAF";
    }
    else if (node_type == NodeType::BRANCH)
    {
        out<<"BRANCH";
    }
    else {
        out<<"UNKNOWN["<<(int)node_type<<"]";
    }

    return out;
}

}
}
}
