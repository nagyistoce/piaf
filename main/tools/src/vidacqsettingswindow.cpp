#include "inc/vidacqsettingswindow.h"
#include "ui_vidacqsettingswindow.h"

VidAcqSettingsWindow::VidAcqSettingsWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::VidAcqSettingsWindow)
{
    ui->setupUi(this);
}

VidAcqSettingsWindow::~VidAcqSettingsWindow()
{
    delete ui;
}

void VidAcqSettingsWindow::changeEvent(QEvent *e)
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
