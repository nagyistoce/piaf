#include "swtoolmainwindow.h"
#include "ui_swtoolmainwindow.h"

SwToolMainWindow::SwToolMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SwToolMainWindow)
{
    ui->setupUi(this);
}

SwToolMainWindow::~SwToolMainWindow()
{
    delete ui;
}

void SwToolMainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
