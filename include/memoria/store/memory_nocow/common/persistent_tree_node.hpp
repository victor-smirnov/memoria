
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

#include <memoria/core/types.hpp>
#include <memoria/core/tools/stream.hpp>
#include <memoria/core/tools/md5.hpp>

#include <atomic>
#include <mutex>

namespace memoria {
namespace store {
namespace memory_nocow {

enum class NodeType {LEAF, BRANCH};

template <typename Key_, int32_t NodeSize, int32_t NodeIndexSize, typename NodeId_, typename SnapshotID_>
class NodeBase {
public:

    using MutexT = std::recursive_mutex;
	using LockGuardT = std::lock_guard<MutexT>;

    using Key       = Key_;
    using NodeId    = NodeId_;
    using SnapshotID     = SnapshotID_;
    using RCType	= int64_t;

    class RootMetadata {
        int64_t size_ = 0;
    public:
        int64_t& size() {
            return size_;
        }
        const int64_t& size() const {
            return size_;
        }

        void add_size(int64_t delta) {
            size_ += delta;
        }
    };

    template <typename, typename, typename, typename>
    friend class PersistentTree;

    template <typename Profile>
    friend class ThreadInMemAllocatorImpl;

    template <typename, int32_t, int32_t, typename, typename>
    friend class NodeBase;

private:

    RootMetadata metadata_;

    NodeType node_type_;

    NodeId node_id_;
    SnapshotID  snapshot_id_;

    int32_t size_ = 0;

    int32_t cpu_id_; // not used here...

protected:

    int64_t refs_;
    mutable MutexT mutex_;

private:

    static constexpr int32_t Indexes =  NodeSize / NodeIndexSize + ((NodeSize % NodeIndexSize == 0)?  0 : 1);

    Key index_[Indexes];
    Key keys_ [NodeSize];

public:
    NodeBase() {
    }

    NodeBase(const SnapshotID& snapshot_id, const NodeId& node_id, NodeType node_type):
        node_type_(node_type),
        node_id_(node_id),
        snapshot_id_(snapshot_id),
		refs_(0)
    {
    }

    void copy_data_from(const NodeBase* other) {
        this->metadata_ = other->metadata_;
        this->node_type_ = other->node_type_;
        this->node_id_ = other->node_id_;
        this->snapshot_id_ = other->snapshot_id_;
        this->size_ = other->size_;
        this->cpu_id_ = other->cpu_id_;

        CopyBuffer(other->index_, index_, Indexes);
        CopyBuffer(other->keys_, keys_, NodeSize);
    }


    MutexT& mutex() {return mutex_;}
    MutexT& mutex() const {return mutex_;}

    void lock() {
        mutex_.lock();
    }

    void unlock() {
        mutex_.unlock();
    }

    template <typename Base>
    void populate_as_buffer(const Base* node)
    {
        node_type_  = node->node_type();
        node_id_    = node->node_id();
        snapshot_id_     = node->snapshot_id();

        metadata_   = node->metadata();
        size_       = node->size();
        refs_       = node->references();

        for (int32_t c = 0; c < NodeIndexSize; c++)
        {
            index_[c] = node->index_[c];
        }

        for (int32_t c = 0; c < size_; c++)
        {
            keys_[c] = node->key(c);
        }
    }

    RootMetadata& metadata() {
        return metadata_;
    }

    const RootMetadata& metadata() const {
        return metadata_;
    }

    const NodeId& node_id() const {
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

    const SnapshotID& snapshot_id() const {
        return snapshot_id_;
    }

    int32_t size() const {
        return size_;
    };

    int32_t max_size() const {
        return NodeSize;
    }

    int32_t capacity() const {
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

    RCType references() const
    {
        LockGuardT lk(mutex_);
        return refs_;
    }

    void ref()
    {
        LockGuardT lk(mutex_);
    	++refs_;
    }

    RCType unref()
    {
        LockGuardT lk(mutex_);
        auto r = --refs_;
        MEMORIA_ASSERT(r, >=, 0);
        return r;
    }

    const Key& key(int32_t idx) const {
        return keys_[idx];
    }

    Key& key(int32_t idx) {
        return keys_[idx];
    }

    int32_t find_key(const Key& key) const
    {
        int32_t last_idx = size_ / NodeIndexSize + (size_ % NodeIndexSize == 0 ? 0 : 1);

        int32_t idx = -1;
        for (int32_t c = 0; c < last_idx; c++)
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

            for (int32_t c = idx; c < idx + NodeIndexSize; c++)
            {
                if (key <= keys_[c])
                {
                    return c;
                }
            }
        }

        return size_;
    }

    int32_t find_key2(const Key& key) const
    {
        int32_t last_idx = size_ / NodeIndexSize + (size_ % NodeIndexSize == 0 ? 0 : 1);

        int32_t idx = 0;
        for (int32_t c = 0; c < last_idx; c++)
        {
            idx += (key > index_[c]);
        }

        if (idx < last_idx)
        {
            idx *= NodeIndexSize;

            int32_t max = (idx + NodeIndexSize) < size_ ? idx + NodeIndexSize : size_;

            for (int32_t c = idx; c < max; c++)
            {
                idx += (key > keys_[c]);
            }

            return idx;
        }

        return size_;
    }


    int32_t find_key3(const Key& key) const
    {
        return std::distance(keys_, std::lower_bound(keys_, keys_ + size_, key));
    }

    void reindex()
    {
        for (auto& k: index_) k = Key();

        int32_t max = (size_ % NodeIndexSize == 0) ? size_ : (size_ - size_ % NodeIndexSize);

        for (int32_t idx = 0; idx < max; idx += NodeIndexSize)
        {
            index_[idx / NodeIndexSize] = keys_[idx + NodeIndexSize - 1];
        }

        int32_t last_element = size_ - 1;

        index_[last_element / NodeIndexSize] = keys_[last_element];
    }

    void insert_key(int32_t idx, const Key& key)
    {
        shift_right(keys_, idx, idx + 1, size_ - idx);
        size_ ++;

        keys_[idx] = key;

        reindex();
    }

    void remove_keys(int32_t start, int32_t end)
    {
        shift_left(keys_, end, start, size_ - end);

        for (int32_t c = size() - (end - start); c < size(); c++) {
            keys_[c] = Key();
        }

        size_ -= (end - start);
        reindex();
    }

    void split_keys_to(int32_t start, NodeBase* other)
    {
        copy_to(keys_, start, other->keys_, 0, size_ - start);

        for (int32_t c = start; c < size(); c++)
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
        out << "NodeType: " << node_type() << std::endl;
        out << "Size: " << size_ << std::endl;
        out << "NodeId: " << node_id_ << std::endl;
        out << "SnapshotID: " << snapshot_id_ << std::endl;
        out << "Refs: " << refs_ << std::endl;

        out << "Index: " << std::endl;

        int32_t last_idx = size_ / NodeIndexSize + (size_ % NodeIndexSize == 0 ? 0 : 1);

        for (int32_t c = 0; c < last_idx; c++)
        {
            out << c << ": " << index_[c] << std::endl;
        }
    }

    void write(OutputStreamHandler& out) const
    {
        out << this->metadata().size();

        out << (int32_t)this->node_type();
        out << this->node_id();
        out << this->snapshot_id();
        out << this->size();
        out << this->references();

        int32_t last_idx = size_ / NodeIndexSize + (size_ % NodeIndexSize == 0 ? 0 : 1);
        for (int32_t c = 0; c < last_idx; c++)
        {
            out << index_[c];
        }

        for (int32_t c = 0; c < size_; c++)
        {
            out << keys_[c];
        }
    }

    void read(InputStreamHandler& in)
    {
        in >> this->metadata().size();

        int node_type;
        in >> node_type;

        this->node_type_ = (NodeType)node_type;

        in >> node_id_;
        in >> snapshot_id_;
        in >> size_;

        in >> refs_;

        int32_t last_idx = size_ / NodeIndexSize + (size_ % NodeIndexSize == 0 ? 0 : 1);
        for (int32_t c = 0; c < last_idx; c++)
        {
            in >> index_[c];
        }

        for (int32_t c = 0; c < size_; c++)
        {
            in >> keys_[c];
        }
    }


protected:
    template <typename T>
    void shift_right(T* data, int32_t from, int32_t to, int32_t size) const
    {
        CopyBuffer(data + from, data + to, size);
    }

    template <typename T>
    void shift_left(T* data, int32_t from, int32_t to, int32_t size) const
    {
        CopyBuffer(data + from, data + to, size);
    }

    template <typename T>
    void copy_to(const T* src, int32_t from, T* dst, int32_t to, int32_t size) const
    {
        CopyBuffer(src + from, dst + to, size);
    }

    void set_snapshot_id(const SnapshotID& snapshot_id) {
        this->snapshot_id_ = snapshot_id;
    }

    void set_node_id(const NodeId& id) {
        this->node_id_ = id;
    }

    void clear_refs() {
        refs_ = 0; // FIXME: LOCK!!!!
    }

    void set_refs(int32_t refs) {
        this->refs_ = refs; // FIXME: LOCK!!!!
    }

    void hash(MD5Hash& md5) const
    {
        md5.add((uint32_t)node_type_);
        md5.add_ubi(metadata_.size());
        md5.add_ubi(node_id_);
        md5.add_ubi(snapshot_id_);
        md5.add_ubi(size_);

        for (int32_t c = 0; c < size_; c++) {
            md5.add_ubi(keys_[c]);
        }
    }
};


template <typename Key, typename Data, int32_t NodeSize, int32_t NodeIndexSize, typename NodeId, typename SnapshotID>
class Node: public NodeBase<Key, NodeSize, NodeIndexSize, NodeId, SnapshotID> {
    using Base = NodeBase<Key, NodeSize, NodeIndexSize, NodeId, SnapshotID>;

protected:
    Data data_[NodeSize];

public:
    using MyType    = Node<Key, Data, NodeSize, NodeIndexSize, NodeId, SnapshotID>;
    using NodeBaseT = Base;

    Node() {}

    Node(const SnapshotID& snapshot_id, const NodeId& node_id, NodeType node_type):
        Base(snapshot_id, node_id, node_type)
    {}

    const Data& data(int32_t idx) const {
        return data_[idx];
    }

    Data& data(int32_t idx) {
        return data_[idx];
    }

    void insert(int32_t idx, const Key& key, const Data& child)
    {
        this->shift_right(data_, idx, idx + 1, this->size() - idx);

        data_[idx] = child;

        this->insert_key(idx, key);
    }

    void remove(int32_t idx) {
        remove(idx, idx + 1);
    }

    void remove(int32_t start, int32_t end)
    {
        auto size = this->size();

        this->shift_left(data_, end, start, size - end);

        for (int32_t c = size - (end - start); c < size; c++) {
            data_[c] = Data();
        }

        this->remove_keys(start, end);
    }

    void split_to(int32_t start, MyType* other)
    {
        this->copy_to(data_, start, other->data_, 0, this->size() - start);

        for (int32_t c = start; c < this->size(); c++)
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

    void copy_data_from(const MyType* other)
    {
        CopyBuffer(other->data_, data_, NodeSize);
        Base::copy_data_from(other);
    }

    uint64_t hash() const
    {
        MD5Hash md5;

        md5.add_ubi((uint64_t)this);

        Base::hash(md5);

        for (int32_t c = 0; c < this->size(); c++) {
            md5.add_ubi((uint64_t)this->data(c));
        }

        return md5.result().hash64();
    }

    void write(OutputStreamHandler& out) const
    {
        Base::write(out);

        for (int32_t c = 0; c < this->size(); c++)
        {
            out << data_[c];
        }
    }

    void read(InputStreamHandler& in)
    {
        Base::read(in);

        for (int32_t c = 0; c < this->size(); c++)
        {
            in >> data_[c];
        }
    }
};


template <
    typename Key,
    int32_t NodeSize,
    int32_t NodeIndexSize,
    typename NodeId,
    typename SnapshotID,
    typename ChildPtrType = NodeBase<Key, NodeSize, NodeIndexSize, NodeId, SnapshotID>*
    >
class BranchNode: public Node<Key, ChildPtrType, NodeSize, NodeIndexSize, NodeId, SnapshotID> {
    using Base = Node<Key, ChildPtrType, NodeSize, NodeIndexSize, NodeId, SnapshotID>;

public:
    using NodeBaseT = NodeBase<Key, NodeSize, NodeIndexSize, NodeId, SnapshotID>;

    BranchNode() {}

    BranchNode(const SnapshotID& snapshot_id, const NodeId& node_id):
        Base(snapshot_id, node_id, NodeType::BRANCH)
    {}

    ~BranchNode() {}


    void del() const {
        delete this;
    }

    NodeBaseT* find_child(const Key& key) const
    {
        int32_t idx = Base::find_key(key);

        if (idx == this->size())
        {
            idx = this->size() - 1;
        }

        return this->data_[idx];
    }

    int32_t find_child_node(const NodeBaseT* child) const
    {
        for (int32_t c = 0; c < this->size(); c++)
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

        out<<"Data: " << std::endl;

        for (int32_t c = 0; c < this->size(); c++)
        {
            auto node = this->data(c);
            out << c << ": " << this->key(c) << " = " << node << " (" << node->node_id() << ", " << node->snapshot_id() << ")" << std::endl;
        }

        out << std::endl;
    }
};



template <typename Key, typename Value_, int32_t NodeSize, int32_t NodeIndexSize, typename NodeId, typename SnapshotID>
class LeafNode: public Node<Key, Value_, NodeSize, NodeIndexSize, NodeId, SnapshotID> {
    using Base = Node<Key, Value_, NodeSize, NodeIndexSize, NodeId, SnapshotID>;

public:
    using Value = Value_;

    LeafNode() {}
    LeafNode(const SnapshotID& snapshot_id, const NodeId& node_id):
        Base(snapshot_id, node_id, NodeType::LEAF)
    {}

    void del() const {
        delete this;
    }

    int32_t find(const Key& key) const
    {
        return this->find_key(key);
    }

    void dump(std::ostream& out = std::cout) const
    {
        Base::dump(out);

        out << "Data: " << std::endl;

        for (int32_t c = 0; c < this->size(); c++)
        {
            out << c << ": " << this->key(c) << " = " << this->data(c) << std::endl;
        }

        out << std::endl;
    }
};


static std::ostream& operator<<(std::ostream& out, NodeType node_type) {
    if (node_type == NodeType::LEAF) {
        out << "LEAF";
    }
    else if (node_type == NodeType::BRANCH)
    {
        out << "BRANCH";
    }
    else {
        out << "UNKNOWN[" << (int)node_type << "]";
    }

    return out;
}

}
}}
