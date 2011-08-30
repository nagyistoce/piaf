#include "inc/multitoolbarsstackedwidget.h"
#include "ui_multitoolbarsstackedwidget.h"

MultiToolbarsStackedWidget::MultiToolbarsStackedWidget(QWidget *parent) :
    QStackedWidget(parent),
    ui(new Ui::MultiToolbarsStackedWidget)
{
    ui->setupUi(this);
}

MultiToolbarsStackedWidget::~MultiToolbarsStackedWidget()
{
    delete ui;
}

void MultiToolbarsStackedWidget::changeEvent(QEvent *e)
{
    QStackedWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
