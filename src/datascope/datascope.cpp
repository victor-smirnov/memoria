
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

#include <memoria/v1/profiles/default/default.hpp>

#include <memoria/v1/memoria.hpp>

#include <memoria/v1/reactor/application.hpp>
#include <memoria/v1/reactor/qt_support.hpp>

#include <memoria/v1/core/graph/graph.hpp>
#include <memoria/v1/core/tools/fixed_array.hpp>

#include <QApplication>
#include <QMainWindow>
#include <QWindow>
#include <QPushButton>
#include <QPlainTextEdit>

#include <iostream>

#include <ui_main_window.h>
#include "main_window.hpp"


using namespace memoria::v1;
using namespace memoria::v1::reactor;

int main(int argc, char** argv, char** envp)
{
    StaticLibraryCtrs<>::init();

    QApplication q_app(argc, argv);
    Application app(argc, argv, envp);

    app.start_engines();

    MainWindow mwin;
    mwin.show();


    return app.run([&]{
        return QtEventLoopFiberFn()();
    });
}


