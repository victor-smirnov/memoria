
//          Copyright Oliver Kowalke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MEMORIA_FIBERS_NUMA_ALGO_WORK_STEALING_H
#define MEMORIA_FIBERS_NUMA_ALGO_WORK_STEALING_H

#include <condition_variable>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>

#include <memoria/fiber/algo/algorithm.hpp>
#include <memoria/fiber/context.hpp>
#include <memoria/fiber/detail/config.hpp>
#include <memoria/fiber/detail/context_spinlock_queue.hpp>
#include <memoria/fiber/detail/context_spmc_queue.hpp>
#include <memoria/fiber/numa/pin_thread.hpp>
#include <memoria/fiber/numa/topology.hpp>
#include <memoria/fiber/scheduler.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include MEMORIA_BOOST_ABI_PREFIX
#endif

namespace memoria {
namespace fibers {
namespace numa {
namespace algo {

class work_stealing : public memoria::fibers::algo::algorithm {
private:
    static std::vector< boost::intrusive_ptr< work_stealing > >    schedulers_;

    std::uint32_t                                           cpu_id_;
    std::vector< std::uint32_t >                            local_cpus_;
    std::vector< std::uint32_t >                            remote_cpus_;
#ifdef MEMORIA_FIBERS_USE_SPMC_QUEUE
    detail::context_spmc_queue                              rqueue_{};
#else
    detail::context_spinlock_queue                          rqueue_{};
#endif
    std::mutex                                              mtx_{};
    std::condition_variable                                 cnd_{};
    bool                                                    flag_{ false };
    bool                                                    suspend_;

    static void init_( std::vector< memoria::fibers::numa::node > const&,
                       std::vector< boost::intrusive_ptr< work_stealing > > &);

public:
    work_stealing( std::uint32_t, std::uint32_t,
                   std::vector< memoria::fibers::numa::node > const&,
                   bool = false);

    work_stealing( work_stealing const&) = delete;
    work_stealing( work_stealing &&) = delete;

    work_stealing & operator=( work_stealing const&) = delete;
    work_stealing & operator=( work_stealing &&) = delete;

    virtual void awakened( context *) noexcept;

    virtual context * pick_next() noexcept;

    virtual context * steal() noexcept {
        return rqueue_.steal();
    }

    virtual bool has_ready_fibers() const noexcept {
        return ! rqueue_.empty();
    }

    virtual void suspend_until( std::chrono::steady_clock::time_point const&) noexcept;

    virtual void notify() noexcept;
};

}}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include MEMORIA_BOOST_ABI_SUFFIX
#endif

#endif // MEMORIA_FIBERS_NUMA_ALGO_WORK_STEALING_H
