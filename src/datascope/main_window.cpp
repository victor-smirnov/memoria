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
#include <QPlainTextDocumentLayout>
#include <QFileDialog>

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
    view->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(exitAction, &QAction::triggered, this, &MainWindow::quit);

    connect(view->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::update_actions);

    connect(view->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::item_selected);

    connect(view, &QTreeView::customContextMenuRequested, this, &MainWindow::open_context_menu);

    connect(createAllocatorAction, &QAction::triggered, this, &MainWindow::create_allocator);

    update_actions();
}


void MainWindow::create_allocator()
{
    AllocatorModel* model = static_cast<AllocatorModel*>(view->model());
    model->createNewInMemAllocator();
}


void MainWindow::open_allocator()
{
    QString fileName = QFileDialog::getOpenFileName(
            this, ("Open File"),
            ".",
            ("Memoria Files (*.mma1)")
    );

    if (!fileName.isNull())
    {
        AllocatorModel* model = static_cast<AllocatorModel*>(view->model());
        model->open_allocator(fileName);
    }
}



void MainWindow::update_actions()
{

}

void MainWindow::open_context_menu(const QPoint& point_at)
{
    QMenu contextMenu(tr("Context menu"), this);

    QAction action1("Open InMem Allocator", this);
    connect(&action1, &QAction::triggered, this, &MainWindow::open_allocator);
    contextMenu.addAction(&action1);

    contextMenu.exec(mapToGlobal(point_at));
}

void MainWindow::item_selected()
{
    auto selection = view->selectionModel()->selectedIndexes();
    if (selection.size() > 0)
    {
        auto item_idx = selection[0];
        AllocatorModel* model = static_cast<AllocatorModel*>(view->model());
        AbstractTreeItem* item = model->get_item(item_idx);

        if (item->node_type() == u"page")
        {
            VertexTreeItem* vx_item = static_cast<VertexTreeItem*>(item);
            VertexProperty content_prop = vx_item->vertex().property(u"content");

            if (!content_prop.is_empty())
            {
                U16String text = boost::any_cast<U16String>(content_prop.value());

                plainTextEdit->clear();
                plainTextEdit->insertPlainText(QString::fromUtf16(text.data()));
            }
        }
        else {
            plainTextEdit->clear();
        }
    }
}

void MainWindow::closeEvent(QCloseEvent*) {
    memoria::v1::reactor::app().shutdown();
}

void MainWindow::quit()
{
    QApplication::quit();
    memoria::v1::reactor::app().shutdown();
}



}}
