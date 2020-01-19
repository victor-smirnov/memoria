
// Copyright 2018 Victor Smirnov
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
#include <memoria/reactor/application.hpp>
#include <memoria/core/memory/smart_ptrs.hpp>

#include <QApplication>
#include <QAbstractEventDispatcher>
#include <QThread>

namespace memoria {
namespace reactor {

enum class QtEventLoopStatus {
    RUNNING, IDLE, TERMINATE
};


class QtEventLoopFiberFn {
    QAbstractEventDispatcher* disp_;
public:
    QtEventLoopFiberFn():
        disp_(QAbstractEventDispatcher::instance())
    {}

    int operator()()
    {
        ShutdownOnScopeExit hh;
        engine().start_service_fiber();

        auto dtr = MakeOnScopeExit([]{
            engine().stop_service_fiber();
        });

        while (process_qt_events() != QtEventLoopStatus::TERMINATE && !engine().shutdown_requested()){
            this_fiber::yield();
        }

        return engine().exit_status();
    }

private:
    QtEventLoopStatus process_qt_events()
    {
        return disp_->processEvents(QEventLoop::AllEvents) ?
                    QtEventLoopStatus::RUNNING :
                    QtEventLoopStatus::IDLE;

    }
};

}}
