
#include <memoria/reactor/application.hpp>
#include <memoria/reactor/qt_support.hpp>

#include <QApplication>
#include <QMainWindow>
#include <QWindow>
#include <QPushButton>
#include <QPlainTextEdit>

#include <iostream>

using namespace memoria::v1;
using namespace memoria::reactor;

class MainWindow: public QMainWindow {
    Q_OBJECT
    QPushButton* button_;
public:

    virtual ~MainWindow(){}

    MainWindow(): button_(new QPushButton(this))
    {
        setCentralWidget(button_);

        connect(button_,
               &QPushButton::clicked,
               this,
               &MainWindow::onPress
        );
    }

    void closeEvent(QCloseEvent *event) {
        app().shutdown();
    }

    void onPress()
    {
        engine().coutln("My First App!!! {}", 12345);
    }
};


#include <qt_app.moc>

int main(int argc, char** argv, char** envp)
{
    QApplication q_app(argc, argv);
    Application app(argc, argv, envp);

    app.start_engines();

    MainWindow mwin;

    mwin.show();

    return app.run(QtEventLoopFiberFn());
}
