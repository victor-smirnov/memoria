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


#include <memoria/reactor/application.hpp>

#include "main_window.hpp"
#include "allocator_model.hpp"

#include <QFile>
#include <QPlainTextDocumentLayout>
#include <QFileDialog>

namespace memoria {

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

    //connect(createAllocatorAction, &QAction::triggered, this, &MainWindow::create_allocator);
    connect(closeAllocatorAction, &QAction::triggered, this, &MainWindow::close_store);
    connect(openAllocatorAction, &QAction::triggered, this, &MainWindow::open_store);

    update_actions();
}


void MainWindow::open_store()
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

void MainWindow::close_store()
{
    auto selection = view->selectionModel()->selectedIndexes();
    closeAllocatorAction->setEnabled(selection.size() > 0);

    if (selection.size() > 0)
    {
        auto item_idx = selection[0];
        AllocatorModel* model = static_cast<AllocatorModel*>(view->model());
        AbstractTreeItem* item = model->get_item(item_idx);

        AbstractTreeItem* top_level_item = model->get_top_level(item);

        model->remove(top_level_item);
        view->selectionModel()->clearSelection();

        delete top_level_item;

        update_actions();
    }
}


void MainWindow::update_actions()
{
    auto selection = view->selectionModel()->selectedIndexes();
    closeAllocatorAction->setEnabled(selection.size() > 0);

    if (selection.size() > 0)
    {
        auto item_idx = selection[0];
        AllocatorModel* model = static_cast<AllocatorModel*>(view->model());
        AbstractTreeItem* item = model->get_item(item_idx);

        AbstractTreeItem* top_level_item = model->get_top_level(item);
        closeAllocatorAction->setEnabled(top_level_item->node_type() == "inmem_allocator");
    }
    else {
        closeAllocatorAction->setEnabled(false);
        plainTextEdit->clear();
    }
}

void MainWindow::open_context_menu(const QPoint& point_at)
{
    QMenu contextMenu(tr("Context menu"), this);

    contextMenu.addAction(openAllocatorAction);
    contextMenu.addAction(closeAllocatorAction);

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

        if (item->node_type() == "block")
        {
            CtrBlockTreeItem* block_item = static_cast<CtrBlockTreeItem*>(item);

            std::stringstream ss;
            block_item->block()->describe(ss);

            plainTextEdit->clear();
            plainTextEdit->insertPlainText(QString::fromUtf8(ss.str().data()));
        }
        else {
            plainTextEdit->clear();
        }
    }
}

void MainWindow::closeEvent(QCloseEvent*) {
    memoria::reactor::app().shutdown();
}

void MainWindow::quit()
{
    QApplication::quit();
    memoria::reactor::app().shutdown();
}



}
