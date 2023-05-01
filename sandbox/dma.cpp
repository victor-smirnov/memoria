
// Copyright 2023 Victor Smirnov
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


#include <memoria/core/tools/time.hpp>

#include <seastar/core/app-template.hh>
#include <seastar/core/reactor.hh>

#include <seastar/core/file.hh>

#include <seastar/core/thread.hh>
#include <seastar/util/closeable.hh>
#include <seastar/core/align.hh>

#include <iostream>

#include <sys/mman.h>


using namespace seastar;

int main(int argc, char** argv)
{
    app_template::seastar_options opts;
    opts.smp_opts.smp.set_value(1);

    std::string storage = ".";

    app_template app(std::move(opts));
    app.run(argc, argv, [&] {
        return seastar::async([&] {
            file ff = engine().open_file_dma(storage + "/file.bin", open_flags::rw | open_flags::exclusive).get();

            size_t size = 1024 * 1024 * 1024;
            auto buf = allocate_aligned_buffer<uint8_t>(size, 4096);

            int64_t t0 = memoria::getTimeInMillis();

            for (size_t c = 0; c < size / 4096; c++) {
                mprotect(buf.get() + (c * 4096), 4096, PROT_READ | PROT_WRITE);
            }

            int64_t t1 = memoria::getTimeInMillis();

            size_t sz = ff.dma_read(0, buf.get(), size).get();

            int64_t t2 = memoria::getTimeInMillis();


            //std::cout << "Size: " << sz << std::endl;

            std::cout << "mprotected in " << memoria::FormatTime(t1 - t0) << std::endl;
            std::cout << "read in " << memoria::FormatTime(t2 - t1) << " size = " << sz << std::endl;
        });
        return seastar::make_ready_future<>();
    });
}
