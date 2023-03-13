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

#include <memoria/asio/reactor.hpp>
#include <memoria/asio/round_robin.hpp>
#include <memoria/tests/runner/process.hpp>
#include <memoria/core/strings/format.hpp>

#include "scroller.hpp"

#include <boost/fiber/all.hpp>
#include <boost/bind.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <ftxui/component/component.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/dom/elements.hpp>

#include <memory>

using namespace memoria;
using namespace memoria::asio;
using namespace memoria::tests;

using namespace ftxui;


namespace ba    = boost::asio;
namespace bf    = boost::fibers;
namespace bsys  = boost::system;

class Worker {
    int num_;
    U8String task_name_;
    uint64_t total_;
    uint64_t progress_;
    bool selected_{};
public:
    Worker(int num): num_(num), total_(1), progress_(0) {
        task_name_ = U8String("Task ") + std::to_string(num);
    }

    size_t num() const {return num_;}

    U8String task_name() const {return task_name_;}
    void set_task_name(U8String task_name) {
        task_name_ = task_name;
    }

    uint64_t total() const {return total_;}
    uint64_t progress() const {return progress_;}

    void set_total(uint64_t val) {
        total_ = val;
    }

    void set_progress(uint64_t val) {
        progress_ = val;
    }

    bool& selected() {return selected_;}
    const bool& selected() const {return selected_;}
};


class Execution {
    std::vector<Worker> workers_;

    size_t total_{};

    size_t failed_{};
    size_t crashed_{};
    size_t passed_{};

public:
    Execution(size_t workers) {
        for (size_t c = 0; c < workers; c++) {
            workers_.emplace_back(c);
        }
    }

    size_t workers() const {return workers_.size();}

    const Worker& worker(size_t num) const {
        return workers_.at(num);
    }

    Worker& worker(size_t num) {
        return workers_.at(num);
    }

    std::vector<std::string> details() const {
        return std::vector<std::string>{};
    }

    size_t total() const {return total_;}
    size_t progress() const {return failed_ + crashed_ + passed_;}
    size_t passed() const {return passed_;}
    size_t failed() const {return failed_;}
    size_t crashed() const {return crashed_;}


    void set_total(size_t val) {total_ = val;}
    //void set_progress(size_t val) {progress_ = val;}
};

std::shared_ptr<Execution> execution;

boost::posix_time::milliseconds refresh_timeout(10);

void refresh_loop(
        const bsys::error_code err,
        ba::deadline_timer* ftxiu_timer,
        ftxui::Loop* loop
) {
    if (!loop->HasQuitted()) {
        loop->RunOnce();

        ftxiu_timer->expires_at(ftxiu_timer->expires_at() + refresh_timeout);
        ftxiu_timer->async_wait(boost::bind(
            refresh_loop,
            boost::asio::placeholders::error,
            ftxiu_timer,
            loop
        ));
    }
    else {
        memoria::asio::io_context().stop();
    }
}


int tests_main(int argc, const char* argv[], const char* envp[])
{
    IOContextPtr io_ctx = std::make_shared<IOContext>();
    set_io_context(io_ctx);

    execution = std::make_shared<Execution>(std::thread::hardware_concurrency());
    boost::fibers::use_scheduling_algorithm< memoria::asio::round_robin>(io_ctx);

    auto to_text = [](int number) {
        return text(std::to_string(number)) | size(WIDTH, EQUAL, 3);
    };

    auto render_worker = [&](const Worker& task) {
        double g_val = task.total() > 0 ?
                    task.progress() / double(task.total()) :
                    1;

        return hbox({
                to_text(task.num()) | color(Color::Yellow),
                text(task.task_name()) | size(WIDTH, EQUAL, 32),
                separator(),
                gauge(g_val),
        });
    };

    auto render_summary = [=]() {
        auto elements = Elements({});

        elements.push_back(hbox({
            text("- total:   "),
            to_text(execution->total()) | bold,
        }) | color(Color::White));

        elements.push_back(hbox({
                text("- done:    "),
                to_text(execution->progress()) | bold,
        }) | color(Color::White));

        elements.push_back(hbox({
                text("- passed:  "),
                to_text(execution->passed()) | bold,
        }) | color(Color::Green));

        if (execution->crashed() + execution->failed() > 0)
        {
            elements.push_back(hbox({
                text("- failed:  "),
                to_text(execution->failed()) | bold,
            }) | color(Color::RedLight));

            elements.push_back(hbox({
                text("- crashed: "),
                to_text(execution->crashed()) | bold,
            }) | color(Color::RedLight));
        }

        auto summary = vbox(elements);
        return window(text(" Summary "), summary);
    };

    auto workers_list = Container::Vertical({});
    for (size_t c = 0; c < execution->workers(); c++)
    {
        auto worker = Renderer([=](bool focused){
            return render_worker(execution->worker(c));
        });
        workers_list->Add(worker);
    }

    auto scroller_wl = Scroller(workers_list);

    auto component = Container::Vertical({});
    component->Add(Renderer(scroller_wl, [=]{
        return window(text("Workers"), scroller_wl->Render() | size(HEIGHT, LESS_THAN, 33) | size(HEIGHT, GREATER_THAN, 7) );
    }));

    auto details = Container::Vertical({});

    for (const auto& detail: execution->details()) {
        details->Add(Renderer([=]{
            return text(detail);
        }));
    }

    auto details_wl = Scroller(details);

    component->Add(Renderer(details_wl, [=]{
        Element progress;

        double progress_val = execution->total() > 0 ?
                    execution->progress() / execution->total() :
                    0.7;

        bool green = execution->failed() + execution->crashed() == 0;

        if (green) {
            progress = gauge(progress_val) | color(Color::Green);
        }
        else {
            progress = gauge(progress_val) | color(Color::Red);
        }

        return hbox({
            render_summary(),
            flex(vbox({
                window(text("Progress"), progress),
                window(text("Details"), details_wl->Render() | color(Color::Red) | size(HEIGHT, GREATER_THAN, 7)),
            }))
        });
    }));

    auto screen = ScreenInteractive::Fullscreen();
    Loop loop(&screen, component);

    ba::deadline_timer ftxiu_timer(*io_ctx, refresh_timeout);
    ftxiu_timer.async_wait(boost::bind(
        refresh_loop,
        boost::asio::placeholders::error,
        &ftxiu_timer,
        &loop
    ));

    io_ctx->run();

    return EXIT_SUCCESS;
}

