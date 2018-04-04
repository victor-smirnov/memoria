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


#include <memoria/v1/reactor/application.hpp>

#include "main_window.hpp"
#include "allocator_model.hpp"

#include <QFile>

namespace memoria {
namespace v1 {


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi(this);

    QStringList headers;
    headers << tr("Type") << tr("Name/ID") << tr("Details");

    AllocatorModel *model = new AllocatorModel(headers);

    view->setModel(model);

    connect(exitAction, &QAction::triggered, this, &MainWindow::quit);

    connect(view->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::updateActions);

    connect(createAllocatorAction, &QAction::triggered, this, &MainWindow::createAllocator);

    updateActions();
}


void MainWindow::createAllocator()
{
    AllocatorModel* model = static_cast<AllocatorModel*>(view->model());
    model->createNewInMemAllocator();

    view->reset();
}


void MainWindow::updateActions()
{

}

void MainWindow::closeEvent(QCloseEvent*) {
    memoria::v1::reactor::app().shutdown();
}

void MainWindow::quit() {
    QApplication::quit();
    memoria::v1::reactor::app().shutdown();
}



}}
