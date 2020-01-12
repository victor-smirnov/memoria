

//          Copyright Oliver Kowalke 2014.
//          Copyright Victor Smirnov 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/tools/ptr_cast.hpp>

#include "protected_fixedsize_stack.hpp"


#include <boost/intrusive/list.hpp>

#include <stdint.h>
#include <cstring>

namespace memoria {
namespace v1 {
namespace fibers {

class protected_stack_pool {
    
    class storage {
        struct ListNode: boost::intrusive::list_base_hook<> {
            memoria::v1::context::stack_context stack_ctx_;
        };
        
        size_t min_pool_size_;
        protected_fixedsize_stack  allocator_;
        
        boost::intrusive::list<ListNode> stack_list_;
        
        size_t use_count_{0};
        
    public:
        storage(size_t min_pool_size, size_t stack_size): 
            min_pool_size_(min_pool_size),
            allocator_(stack_size) 
        {
            for (size_t c = 0; c < min_pool_size; c++)
            {
                auto ctx = create_new_node();
                deallocate(ctx);
            }
        }
        
        ~storage() noexcept {
            stack_list_.erase_and_dispose(
                stack_list_.begin(),
                stack_list_.end(),
                [this](ListNode* node) {
                    allocator_.deallocate(node->stack_ctx_);
                }
            );
        }
        
        memoria::v1::context::stack_context allocate()
        {
            if (stack_list_.size() > 0) 
            {
                ListNode* node = &stack_list_.back();
                stack_list_.pop_back();
                
                memoria::v1::context::stack_context ctx(node->stack_ctx_);
                ctx.sp = tools::ptr_cast<uint8_t>(ctx.sp) - sizeof(ListNode);
                ctx.size -= sizeof(ListNode);
                
                return ctx;
            }
            else {
                return create_new_node();
            }
        }
        
        
        
        void deallocate( memoria::v1::context::stack_context & sctx) noexcept 
        {
            ListNode* node = tools::ptr_cast<ListNode>(sctx.sp);
            
            if (stack_list_.size() < min_pool_size_)
            {
                stack_list_.push_back(*node);
            }
            else {
                allocator_.deallocate(node->stack_ctx_);
            }
        }
        
        friend void intrusive_ptr_add_ref( storage * s) noexcept {
            ++s->use_count_;
        }

        friend void intrusive_ptr_release( storage * s) noexcept {
            if ( 0 == --s->use_count_) {
                delete s;
            }
        }
        
    private:
        
        memoria::v1::context::stack_context create_new_node()
        {
            auto ctx = allocator_.allocate();
            
            size_t node_size = sizeof(ListNode);
            
            ListNode* node = tools::ptr_cast<ListNode>(tools::ptr_cast<uint8_t>(ctx.sp) - node_size);
            
            std::memset(node, 0, node_size);
            
            node->stack_ctx_ = ctx;
            
            ctx.sp = node;
            ctx.size -= node_size;
            
            return ctx;
        }  
    };
    
    ::boost::intrusive_ptr< storage > storage_;
    
public:
    protected_stack_pool(size_t min_pool_size, size_t stack_size = memoria::v1::context::stack_traits::default_size()): 
        storage_(new storage(min_pool_size, stack_size))
    {}
    
    memoria::v1::context::stack_context allocate() {
        return storage_->allocate();
    }
    
    void deallocate( memoria::v1::context::stack_context & sctx) noexcept {
        return storage_->deallocate(sctx);
    }
};

}}}
