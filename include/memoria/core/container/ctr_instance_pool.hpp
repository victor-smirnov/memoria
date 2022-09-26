
// Copyright 2022 Victor Smirnov
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

#include <memoria/core/memory/object_pool.hpp>
#include <memoria/profiles/common/common.hpp>
#include <memoria/core/container/ctr_referenceable.hpp>

#include <memoria/core/tools/result.hpp>

#include <unordered_map>

namespace memoria {

template <typename Profile>
class CtrInstancePool: public std::enable_shared_from_this<CtrInstancePool<Profile>> {
    using CtrID = ProfileCtrID<Profile>;
    using CtrT = CtrReferenceable<ApiProfile<Profile>>;

    using PoolPtr = std::shared_ptr<CtrInstancePool>;

    class CtrRefHolder: public SharedPtrHolder {
        CtrID ctr_id_;
        std::unique_ptr<CtrT> instance_;
        PoolPtr pool_;

    public:
        CtrRefHolder(CtrID ctr_id, std::unique_ptr<CtrT>&& instance) noexcept :
            ctr_id_(ctr_id), instance_(std::move(instance))
        {
            instance_->internal_configure_shared_from_this(this);
            //println("Creating CtrRefHolder: {}", (void*)this);
        }

        CtrRefHolder(CtrRefHolder&& other) noexcept :
            ctr_id_(other.ctr_id_),
            instance_(std::move(other.instance_)),
            pool_(std::move(other.pool_))
        {
            instance_->internal_configure_shared_from_this(this);
            //println("Creating CtrRefHolder: {} from {}", (void*)this, (void*)&other);
        }

        virtual ~CtrRefHolder() noexcept {
            //println("Deleting CtrRefHolder: {}", (void*)this);
        }

        void attach_to_pool(PoolPtr&& pool) {
            pool_ = pool;
        }

        void detouch_from_pool() {
            pool_.reset();
        }

        CtrT* get_instance() const {return instance_.get();}

        bool is_in_use() const {
            return (bool)pool_;
        }

        const CtrID& ctr_id() const {
            return ctr_id_;
        }

        void rename(CtrID& new_id) noexcept {
            ctr_id_ = new_id;
        }

    protected:
        virtual void dispose() noexcept {
            delete this;
        }

        virtual void destroy() noexcept {
            pool_->release_ctr_instance(this);
        }
    };

    std::unordered_map<CtrID, CtrRefHolder*> instances_;

    friend class CtrRefHolder;

public:
    using CtrPtr = PoolSharedPtr<CtrT>;

    CtrInstancePool() {}
    CtrInstancePool(const CtrInstancePool&) = delete;

    ~CtrInstancePool() noexcept {
        for (auto pair: instances_) {
            delete pair.second;
        }
    }

    template <typename StoreT>
    CtrPtr get(const CtrID& ctr_id, StoreT store)
    {
        auto ii = instances_.find(ctr_id);
        if (ii != instances_.end())
        {
            CtrRefHolder* holder = ii->second;
            if (holder->is_in_use()) {
                holder->ref_copy();
            }
            else {
                holder->attach_to_pool(this->shared_from_this());
                holder->get_instance()->internal_attach_to_store(store);                
            }

            return pool::detail::make_shared_ptr_from(holder->get_instance(), holder);
        }
        else {
            return CtrPtr{};
        }
    }

    CtrPtr put_new_instance(
            const CtrID& ctr_id,
            std::unique_ptr<CtrT>&& instance
    )
    {
        std::unique_ptr<CtrRefHolder> holder = std::make_unique<CtrRefHolder>(ctr_id, std::move(instance));
        holder->attach_to_pool(this->shared_from_this());

        CtrRefHolder* hh = holder.get();
        instances_.insert(std::make_pair(ctr_id, hh));

        return pool::detail::make_shared_ptr_from(holder->get_instance(), holder.release());
    }

    void remove(const CtrID& ctr_id)
    {
        auto ii = instances_.find(ctr_id);
        if (ii != instances_.end())
        {
            if (!ii->second->is_in_use()) {
                delete ii->second;
            }

            instances_.erase(ii);
        }
    }

    void rename(const CtrID& old_id, const CtrID& new_id)
    {
        auto ii = instances_.find(old_id);
        if (ii != instances_.end())
        {
            CtrRefHolder* holder = ii.second;
            holder->rename(new_id);
            instances_.erase(ii);
            instances_[new_id] = holder;
        }
    }

    bool contains(const CtrID& ctr_id) const {
        return instances_.count(ctr_id);
    }

    size_t count_open() const
    {
        size_t cnt{};
        for (auto& pair: instances_){
            cnt += pair.second->is_in_use();
        }

        return cnt;
    }


    template<typename StoreT>
    void for_each_ctr(StoreT store, std::function<void (const CtrID&, CtrPtr, bool)> fn) const
    {
        std::vector<std::tuple<CtrID, CtrPtr, bool>> data;

        for (auto& pair: instances_)
        {
            bool in_use = pair.second->is_in_use();

            auto ptr = get(pair.first, store);
            data.emplace_back(pair.first, ptr, in_use);
        }

        for (auto& rec: data) {
            fn(std::get<0>(rec), std::get<1>(rec), std::get<2>(rec));
        }
    }

    template<typename StoreT>
    void for_each_open_ctr(StoreT store, std::function<void (const CtrID&, CtrPtr)> fn)
    {
        std::vector<std::tuple<CtrID, CtrPtr>> data;

        for (auto& pair: instances_)
        {
            bool in_use = pair.second->is_in_use();
            if (in_use) {
                auto ptr = get(pair.first, store);
                data.emplace_back(pair.first, ptr);
            }
        }

        for (auto& rec: data) {
            fn(std::get<0>(rec), std::get<1>(rec));
        }
    }

protected:
    void release_ctr_instance(CtrRefHolder* holder)
    {
        //println("Releasing CtrRefHolder: {}", (void*)holder);

        CtrRefHolder* new_holder = new CtrRefHolder(std::move(*holder));

        new_holder->get_instance()->internal_detouch_from_store();

        instances_[new_holder->ctr_id()] = new_holder;

        new_holder->detouch_from_pool();
    }
};

}
