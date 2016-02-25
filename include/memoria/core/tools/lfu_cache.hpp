
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_TOOLS_LRU_CACHE_HPP_
#define MEMORIA_CORE_TOOLS_LRU_CACHE_HPP_

#include <memoria/core/tools/intrusive_list.hpp>
#include <memoria/core/tools/config.hpp>

#include <memoria/core/exceptions/memoria.hpp>

#include <unordered_map>
#include <functional>
#include <iostream>

namespace memoria {

template <typename Node>
struct DefaultEvictionPredicate {
    bool operator()(const Node& node) {return true;}
};

template <typename Node>
struct DefaultNodeExtender: Node {};

template <typename Key, typename Value>
using DefaultLRUMap = std::unordered_map<Key, Value>;

template <
    typename Key,
    template <typename Node> class EvictionPredicate                = DefaultEvictionPredicate,
    template <typename Base> class NodeExtender                     = DefaultNodeExtender,
    template <typename KeyType, typename ValueType> class Map       = DefaultLRUMap
>
class LFUCache {

    typedef LFUCache<Key, EvictionPredicate, NodeExtender, Map>     MyType;

    template <
        template <typename> class NodeExtender1
    >
    class NodeBase {
        typedef NodeExtender<NodeBase<NodeExtender1>>* NodePtr;

        NodePtr next_;
        NodePtr prev_;

        UBigInt counter_ = 0;

        Key     key_;

        MyType* owner_;

    public:
        NodeBase():
            next_(nullptr), prev_(nullptr)
        {}

        NodePtr& next() {return next_;}
        const NodePtr& next() const {return next_;}

        NodePtr& prev() {return prev_;}
        const NodePtr& prev() const {return prev_;}

        UBigInt& counter() {return counter_;}

        const UBigInt& counter() const {return counter_;}

        Key& key() {return key_;}
        const Key& key() const {return key_;}

        bool is_allocated() const
        {
            return owner_ != nullptr;
        }

        void reset()
        {
            next_ = prev_ = nullptr;

            counter_    = 0;
            owner_      = nullptr;
        }

    //private:
        void set_owner(MyType* owner) {this->owner_ = owner;}
        MyType* owner() {return owner_;}
        const MyType* owner() const {return owner_;}
    };

public:
    typedef NodeExtender<NodeBase<NodeExtender>>    Entry;
    typedef std::function<void (Entry&)>            FillCacheFn;

private:
    typedef Map<Key, Entry*>                        MapType;
    typedef IntrusiveList<Entry>                    ListType;

    MapType     map_;
    ListType    list_;

    std::size_t max_size_;

    EvictionPredicate<Entry> eviction_predicate_;

    std::size_t propagations_ = 0;

public:

    LFUCache(std::size_t max_size): max_size_(max_size) {}

    MapType& map() {
        return map_;
    }

    const MapType& map() const {
        return map_;
    }

    ListType& list() {
        return list_;
    }

    const ListType& list() const {
        return list_;
    }

    std::size_t size() const {
        return map_.size();
    }

    std::size_t max_size() const {
        return max_size_;
    }

    bool contains_key(const Key& key) const
    {
        return map_.find(key) != map_.end();
    }

    bool contains_entry(const Entry* entry) const
    {
        return entry->owner() == this;
    }

    bool insert(Entry* entry)
    {
        MyType* owner = entry->owner();

        if (!contains_entry(entry))
        {
            if (owner)
            {
                owner->remove_entry(entry);
            }

            entry->set_owner(this);

            map_[entry->key()] = entry;
            list_.insert(list_.begin(), entry);

            propagate(entry);

            return true;
        }
        else {
            return false;
        }
    }

    bool touch(Entry* entry)
    {
        if (contains_entry(entry))
        {
            entry->counter()++;
            propagate(entry);

            return true;
        }
        else {
            return false;
        }
    }

    Entry* get_entry(const Key& key, FillCacheFn fill_fn)
    {
        auto iter = map_.find(key);

        if (iter != map_.end())
        {
            Entry* entry = iter->second;
            entry->counter()++;

            propagate(entry);

            return entry;
        }
        else if (size() >= max_size_)
        {
            Entry* entry_to_evict = findNodeToEvict();

            if (entry_to_evict)
            {
                map_.erase(entry_to_evict->key());
                list_.erase(entry_to_evict);

                entry_to_evict->reset();

                entry_to_evict->key() = key;

                entry_to_evict->set_owner(this);

                try {
                    fill_fn(*entry_to_evict);

                    map_[key] = entry_to_evict;
                    list_.insert(list_.begin(), entry_to_evict);

                    return entry_to_evict;
                }
                catch(...)
                {
                    delete entry_to_evict;
                    throw;
                }
            }
            else if (max_size_ > 0)
            {
                throw Exception(MA_SRC, "Nothing to evict");
            }
            else {
                throw Exception(MA_SRC, "The cache is configured empty");
            }
        }
        else {
            Entry* entry    = new Entry();
            entry->key()    = key;
            entry->set_owner(this);

            fill_fn(*entry);

            map_[key] = entry;
            list_.insert(list_.begin(), entry);

            return entry;
        }
    }

    Entry* remove_entry(const Key& key)
    {
        auto i = map_.find(key);

        if (i != map_.end())
        {
            Entry* entry = i->second;

            list_.erase(entry);

            return entry;
        }
        else {
            return nullptr;
        }
    }

    bool remove_entry(Entry* entry)
    {
        if (entry->owner() == this)
        {
            list_.erase(entry);
            map_.erase(entry->key());

            entry->set_owner(nullptr);

            return true;
        }
        else {
            return false;
        }
    }

    void dump() const
    {
        for (auto& entry: list_)
        {
            dump(entry);
        }

        std::cout<<"propagations="<<propagations_<<std::endl;
    }

    void checkOrder() const {

        UBigInt last = 0;

        size_t pos = 0;
        for (auto& entry: list_)
        {
            if (entry.counter() >= last)
            {
                last = entry.counter();
                pos++;
            }
            else {
                std::cout<<"Invalid node order at "<<pos<<std::endl;
                dump(entry);
            }
        }
    }

private:

    Entry* findNodeToEvict()
    {
        for (Entry& entry: list_)
        {
            if (eviction_predicate_(entry))
            {
                return &entry;
            }
        }

//      for (Entry& entry: list_) {
////            cout<<entry.is_updated()<<" "<<entry.shared()<<endl;
//      }

        return nullptr;
    }

    std::size_t propagate(Entry* entry)
    {
        Entry* tmp = entry->next();

        std::size_t distance = 0;

        while (tmp && entry->counter() >= tmp->counter())
        {
            tmp = tmp->next();
            distance++;
            propagations_++;
        }

        if (distance > 0)
        {
            list_.erase(entry);
            list_.insert(typename ListType::iterator(tmp), entry);
        }

        return distance;
    }

    void dump(const Entry& entry) const
    {
        std::cout<<"Node: key="<<entry.key()<<" counter="<<entry.counter()<<std::endl;
    }
};


}


#endif
