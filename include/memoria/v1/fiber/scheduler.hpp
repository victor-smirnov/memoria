//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include <boost/config.hpp>
#include <boost/context/execution_context.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/intrusive/set.hpp>

#include <memoria/v1/fiber/algo/algorithm.hpp>
#include <memoria/v1/fiber/context.hpp>
#include <memoria/v1/fiber/detail/config.hpp>
#include <memoria/v1/fiber/detail/context_mpsc_queue.hpp>
#include <memoria/v1/fiber/detail/data.hpp>
#include <memoria/v1/fiber/detail/spinlock.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:4251)
#endif

namespace memoria {
namespace v1 {    
namespace fibers {

class MEMORIA_V1_FIBERS_DECL scheduler {
public:
    struct timepoint_less {
        bool operator()( context const& l, context const& r) const noexcept {
            return l.tp_ < r.tp_;
        }
    };

    typedef boost::intrusive::list<
                context,
                boost::intrusive::member_hook<
                    context, detail::ready_hook, & context::ready_hook_ >,
                boost::intrusive::constant_time_size< false > >    ready_queue_t;
private:
    typedef boost::intrusive::multiset<
                context,
                boost::intrusive::member_hook<
                    context, detail::sleep_hook, & context::sleep_hook_ >,
                boost::intrusive::constant_time_size< false >,
                boost::intrusive::compare< timepoint_less > >      sleep_queue_t;
    typedef boost::intrusive::list<
                context,
                boost::intrusive::member_hook<
                    context, detail::terminated_hook, & context::terminated_hook_ >,
                boost::intrusive::constant_time_size< false > >    terminated_queue_t;
    typedef boost::intrusive::list<
                context,
                boost::intrusive::member_hook<
                    context, detail::worker_hook, & context::worker_hook_ >,
                boost::intrusive::constant_time_size< false > >    worker_queue_t;

    std::unique_ptr< algo::algorithm >  algo_;
    context                         *   main_ctx_{ nullptr };
    boost::intrusive_ptr< context >            dispatcher_ctx_{};
    // worker-queue contains all context' mananged by this scheduler
    // except main-context and dispatcher-context
    // unlink happens on destruction of a context
    worker_queue_t                      worker_queue_{};
    // terminated-queue contains context' which have been terminated
    terminated_queue_t                  terminated_queue_{};

    // scheduler::wait_until()
    sleep_queue_t                       sleep_queue_{};
    bool                                shutdown_{ false };

    context * get_next_() noexcept;

    void release_terminated_() noexcept;

    void sleep2ready_() noexcept;

public:
    scheduler() noexcept;

    scheduler( scheduler const&) = delete;
    scheduler & operator=( scheduler const&) = delete;

    virtual ~scheduler();

    void set_ready( context *) noexcept;

    boost::context::execution_context< detail::data_t * > dispatch() noexcept;

    boost::context::execution_context< detail::data_t * > set_terminated( context *) noexcept;


    void yield( context *) noexcept;

    bool wait_until( context *,
                     std::chrono::steady_clock::time_point const&) noexcept;
    bool wait_until( context *,
                     std::chrono::steady_clock::time_point const&,
                     detail::spinlock_lock &) noexcept;

    void suspend() noexcept;
    void suspend( detail::spinlock_lock &) noexcept;

    bool has_ready_fibers() const noexcept;

    void set_algo( std::unique_ptr< algo::algorithm >) noexcept;

    void attach_main_context( context *) noexcept;

    void attach_dispatcher_context( boost::intrusive_ptr< context >) noexcept;

    void attach_worker_context( context *) noexcept;

    void detach_worker_context( context *) noexcept;
};

}}}

#ifdef _MSC_VER
# pragma warning(pop)
#endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif


