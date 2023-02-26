#include <memoria/reactor/mpsc_queue.hpp>
#include <memoria/core/tools/time.hpp>
#include <memoria/core/strings/format.hpp>

#include <boost/pool/object_pool.hpp>
#include <memoria/core/memory/object_pool.hpp>
#include <boost/smart_ptr.hpp>

#include <thread>
#include <queue>
#include <set>
#include <unordered_set>

using namespace memoria::reactor;

struct SS {
    uint64_t value;
    uint8_t  pad[128 - 8];
};

int main() {

    using Msg = SS;

    using Pool = boost::object_pool<Msg>;

    Pool pool(1024);

    std::queue<Msg*> qq;

    size_t prefetch_size = 128;
    size_t total = 1000000000;

    int64_t t0 = memoria::getTimeInMillis();

    for (size_t c = 0; c < prefetch_size; c++) {
        qq.push(pool.construct(Msg{c}));
        //qq.push(new Msg{c});
    }

    for (size_t c = prefetch_size; c < total; c++) {
        pool.destroy(qq.front());
        //delete qq.front();
        qq.pop();

        qq.push(pool.construct(Msg{c}));
        //qq.push(new Msg{c});
    }

    for (size_t c = 0; c < qq.size(); c++) {
        pool.destroy(qq.front());
        //delete qq.front();
        qq.pop();
    }

    int64_t t1 = memoria::getTimeInMillis() + 1;

    memoria::println("Totals: {} ms, op/s = {}", t1 - t0, total / (t1 - t0) * 1000);

    return 0;
}
