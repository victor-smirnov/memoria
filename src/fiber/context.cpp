
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "memoria/v1/fiber/context.hpp"

#include <cstdlib>
#include <mutex>
#include <new>
#include <iostream>

#include "memoria/v1/fiber/exceptions.hpp"
#include "memoria/v1/fiber/scheduler.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace memoria {
namespace v1 {    
namespace fibers {

static boost::intrusive_ptr< context > make_dispatcher_context( scheduler * sched) {
    BOOST_ASSERT( nullptr != sched);
    default_stack salloc; // use default satck-size
    boost::context::stack_context sctx = salloc.allocate();

    constexpr std::size_t func_alignment = 64; // alignof( context);
    constexpr std::size_t func_size = sizeof( context);
    // reserve space on stack
    void * sp = static_cast< char * >( sctx.sp) - func_size - func_alignment;
    // align sp pointer
    std::size_t space = func_size + func_alignment;
    sp = std::align( func_alignment, func_size, sp, space);
    BOOST_ASSERT( nullptr != sp);
    // calculate remaining size
    const std::size_t size = sctx.size - ( static_cast< char * >( sctx.sp) - static_cast< char * >( sp) );

    // placement new of context on top of fiber's stack
    return boost::intrusive_ptr< context >( 
        ::new ( sp) context(
                dispatcher_context,
                boost::context::preallocated( sp, size, sctx),
                salloc,
                sched) );
}

// schwarz counter
struct context_initializer {
    static thread_local context *   active_;
    static thread_local std::size_t counter_;
    
    static thread_local std::size_t contexts_;

    context_initializer() {
        if ( 0 == counter_++) {
            constexpr std::size_t alignment = 64; // alignof( capture_t);
            constexpr std::size_t ctx_size = sizeof( context);
            constexpr std::size_t sched_size = sizeof( scheduler);
            constexpr std::size_t size = 2 * alignment + ctx_size + sched_size;
            void * vp = std::malloc( size);
            if ( nullptr == vp) {
                throw std::bad_alloc();
            }
            // reserve space for shift
            void * vp1 = static_cast< char * >( vp) + sizeof( int); 
            // align context pointer
            std::size_t space = ctx_size + alignment;
            vp1 = std::align( alignment, ctx_size, vp1, space);
            // reserves space for integer holding shifted size
            int * shift = reinterpret_cast< int * >( static_cast< char * >( vp1) - sizeof( int) );
            // store shifted size in fornt of context
            * shift = static_cast< char * >( vp1) - static_cast< char * >( vp);
            // main fiber context of this thread
            context * main_ctx = ::new ( vp1) context( main_context);
            vp1 = static_cast< char * >( vp1) + ctx_size;
            // align scheduler pointer
            space = sched_size + alignment;
            vp1 = std::align( alignment, sched_size, vp1, space);
            // scheduler of this thread
            scheduler * sched = ::new ( vp1) scheduler();
            // attach main context to scheduler
            sched->attach_main_context( main_ctx);
            // create and attach dispatcher context to scheduler
            sched->attach_dispatcher_context(
                    make_dispatcher_context( sched) );
            // make main context to active context
            active_ = main_ctx;
        }
    }

    ~context_initializer() {
        if ( 0 == --counter_) {
            context * main_ctx = active_;
            BOOST_ASSERT( main_ctx->is_context( type::main_context) );
            scheduler * sched = main_ctx->get_scheduler();
            sched->~scheduler();
            main_ctx->~context();
            int * shift = reinterpret_cast< int * >( reinterpret_cast< char * >( main_ctx) - sizeof( int) );
            void * vp = reinterpret_cast< char * >( main_ctx) - ( * shift);
            std::free( vp);
        }
    }
};

// zero-initialization
thread_local context * context_initializer::active_;
thread_local std::size_t context_initializer::counter_;
thread_local std::size_t context_initializer::contexts_ = 0;


void
context::suspend() noexcept {
    get_scheduler()->suspend();
}

void
context::suspend( detail::spinlock_lock & lk) noexcept {
    get_scheduler()->suspend( lk);
}


void
context::resume() noexcept {
    context * prev = this;
    // context_initializer::active_ will point to `this`
    // prev will point to previous active context
    std::swap( context_initializer::active_, prev);

    detail::data_t d{ prev };

    resume_( d);
}

void
context::resume( detail::spinlock_lock & lk) noexcept {
    context * prev = this;
    // context_initializer::active_ will point to `this`
    // prev will point to previous active context
    std::swap( context_initializer::active_, prev);
    detail::data_t d{ & lk, prev };
    resume_( d);
}

void
context::resume( context * ready_ctx) noexcept {
    context * prev = this;
    // context_initializer::active_ will point to `this`
    // prev will point to previous active context
    std::swap( context_initializer::active_, prev);
    detail::data_t d{ ready_ctx, prev };

    resume_( d);
}

context *
context::active() noexcept {

    // initialized the first time control passes; per thread
    thread_local static context_initializer ctx_initializer;
    return context_initializer::active_;
}

void
context::reset_active() noexcept {
    context_initializer::active_ = nullptr;
}

size_t context::contexts() noexcept {
    return context_initializer::contexts_;
}


void context::resume_( detail::data_t & d) noexcept 
{
    MMA1_START_SWITCH_FIBER(this->fake_stack_, stack_pointer_, stack_size_);
    
    auto result = ctx_( & d);
    
    MMA1_FINISH_SWITCH_FIBER(this->fake_stack_, this->stack_pointer_, this->stack_size_);
    
    detail::data_t * dp( std::get< 1 >(result) );
    
    if ( nullptr != dp) 
    {
        dp->from->ctx_ = std::move( std::get< 0 >(result) );
        if ( nullptr != dp->lk) 
        {
            dp->lk->unlock();
        } 
        else if ( nullptr != dp->ctx) {
            context_initializer::active_->set_ready_( dp->ctx);
        }
    }
}


void
context::set_ready_( context * ctx) noexcept {
    get_scheduler()->set_ready( ctx);
}

// main fiber context
context::context( main_context_t) noexcept :
    use_count_{ 1 }, // allocated on main- or thread-stack
    flags_{ 0 },
    type_{ type::main_context },
    ctx_{} 
{
    inc_contexts();
}

// dispatcher fiber context
context::context( dispatcher_context_t, boost::context::preallocated const& palloc,
                  default_stack const& salloc, scheduler * sched) :
    flags_{ 0 },
    type_{ type::dispatcher_context },
#ifdef MMA1_SANITIZE_STACKS
    stack_pointer_{palloc.sp},
    stack_size_{palloc.size},
#endif
    ctx_{ std::allocator_arg, palloc, salloc, 
        [this,sched](boost::context::execution_context< detail::data_t * > ctx, detail::data_t * dp) noexcept {
            
            MMA1_FINISH_SWITCH_FIBER(dp->from->fake_stack_, dp->from->stack_pointer_, dp->from->stack_size_);
            
            // update execution_context of calling fiber
            dp->from->ctx_ = std::move( ctx);
            
            if ( nullptr != dp->lk) {
                dp->lk->unlock();
            } 
            else if ( nullptr != dp->ctx) {
                context_initializer::active_->set_ready_( dp->ctx);
            }
            
            // execute scheduler::dispatch()
            return sched->dispatch();
    }}
{
    inc_contexts();
}

context::~context() {
    BOOST_ASSERT( wait_queue_.empty() );
    BOOST_ASSERT( ! ready_is_linked() );
    BOOST_ASSERT( ! sleep_is_linked() );
    BOOST_ASSERT( ! wait_is_linked() );
    delete properties_;
    
    context_initializer::contexts_--;
}

context::id
context::get_id() const noexcept {
    return id( const_cast< context * >( this) );
}



void
context::join() {
    // get active context
    context * active_ctx = context::active();
    // protect for concurrent access
    std::unique_lock< detail::spinlock > lk( splk_);
    // wait for context which is not terminated
    if ( 0 == ( flags_ & flag_terminated) ) {
        // push active context to wait-queue, member
        // of the context which has to be joined by
        // the active context
        active_ctx->wait_link( wait_queue_);
        lk.unlock();
        // suspend active context
        get_scheduler()->suspend();
        // remove from wait-queue
        active_ctx->wait_unlink();
        // active context resumed
        BOOST_ASSERT( context::active() == active_ctx);
    }
}

void
context::yield() noexcept {
    // yield active context
    get_scheduler()->yield( context::active() );
}


boost::context::execution_context< detail::data_t * >
context::suspend_with_cc() noexcept {
    context * prev = this;
    // context_initializer::active_ will point to `this`
    // prev will point to previous active context
    std::swap( context_initializer::active_, prev);
    detail::data_t d{ prev };
    // context switch
    
    MMA1_START_SWITCH_FIBER(this->fake_stack_, stack_pointer_, stack_size_);
    
    return std::move( std::get< 0 >( ctx_( & d) ) );
}

boost::context::execution_context< detail::data_t * >

context::set_terminated() noexcept {
    // protect for concurrent access
    std::unique_lock< detail::spinlock > lk( splk_);
    // mark as terminated
    flags_ |= flag_terminated;
    // notify all waiting fibers
    while ( ! wait_queue_.empty() ) {
        context * ctx = & wait_queue_.front();
        // remove fiber from wait-queue
        wait_queue_.pop_front();
        // notify scheduler
        set_ready( ctx);
    }
    lk.unlock();
    // release fiber-specific-data
    for ( fss_data_t::value_type & data : fss_data_) {
        data.second.do_cleanup();
    }
    fss_data_.clear();
    // switch to another context
    return get_scheduler()->set_terminated( this);
}

bool
context::wait_until( std::chrono::steady_clock::time_point const& tp) noexcept {
    BOOST_ASSERT( nullptr != get_scheduler() );
    BOOST_ASSERT( this == context_initializer::active_);
    return get_scheduler()->wait_until( this, tp);
}

bool
context::wait_until( std::chrono::steady_clock::time_point const& tp,
                     detail::spinlock_lock & lk) noexcept {
    BOOST_ASSERT( nullptr != get_scheduler() );
    BOOST_ASSERT( this == context_initializer::active_);
    return get_scheduler()->wait_until( this, tp, lk);
}

void
context::set_ready( context * ctx) noexcept {
    //BOOST_ASSERT( nullptr != ctx);
    BOOST_ASSERT( this != ctx);
    BOOST_ASSERT( nullptr != get_scheduler() );
    BOOST_ASSERT( nullptr != ctx->get_scheduler() );

    BOOST_ASSERT( get_scheduler() == ctx->get_scheduler() );
    get_scheduler()->set_ready( ctx);
}

void *
context::get_fss_data( void const * vp) const {
    uintptr_t key( reinterpret_cast< uintptr_t >( vp) );
    fss_data_t::const_iterator i( fss_data_.find( key) );
    return fss_data_.end() != i ? i->second.vp : nullptr;
}

void
context::set_fss_data( void const * vp,
                       detail::fss_cleanup_function::ptr_t const& cleanup_fn,
                       void * data,
                       bool cleanup_existing) 
{
    BOOST_ASSERT( cleanup_fn);
    uintptr_t key( reinterpret_cast< uintptr_t >( vp) );
    fss_data_t::iterator i( fss_data_.find( key) );
    if ( fss_data_.end() != i) {
        if( cleanup_existing) {
            i->second.do_cleanup();
        }
        if ( nullptr != data) {
            fss_data_.insert(
                    i,
                    std::make_pair(
                        key,
                        fss_data( data, cleanup_fn) ) );
        } else {
            fss_data_.erase( i);
        }
    } else {
        fss_data_.insert(
            std::make_pair(
                key,
                fss_data( data, cleanup_fn) ) );
    }
}

void
context::set_properties( fiber_properties * props) noexcept {
    delete properties_;
    properties_ = props;
}

bool
context::worker_is_linked() const noexcept {
    return worker_hook_.is_linked();
}

bool
context::terminated_is_linked() const noexcept {
    return terminated_hook_.is_linked();
}

bool
context::ready_is_linked() const noexcept {
    return ready_hook_.is_linked();
}

bool
context::sleep_is_linked() const noexcept {
    return sleep_hook_.is_linked();
}

bool
context::wait_is_linked() const noexcept {
    return wait_hook_.is_linked();
}

void
context::worker_unlink() noexcept {
    worker_hook_.unlink();
}

void
context::ready_unlink() noexcept {
    ready_hook_.unlink();
}

void
context::sleep_unlink() noexcept {
    sleep_hook_.unlink();
}

void
context::wait_unlink() noexcept {
    wait_hook_.unlink();
}

void
context::detach() noexcept {
    BOOST_ASSERT( context::active() != this);
    get_scheduler()->detach_worker_context( this);
}

void
context::attach( context * ctx) noexcept {
    BOOST_ASSERT( nullptr != ctx);
    get_scheduler()->attach_worker_context( ctx);
}

void context::inc_contexts() {
    context_initializer::contexts_++;
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
