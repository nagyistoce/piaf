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

void MultiToolbarsStackedWidget::on_goFirstButton_clicked()
{

}

void MultiToolbarsStackedWidget::on_goPrevButton_clicked()
{

}

void MultiToolbarsStackedWidget::on_playButton_toggled(bool checked)
{

}

void MultiToolbarsStackedWidget::on_goNextButton_clicked()
{

}

void MultiToolbarsStackedWidget::on_goLastButton_clicked()
{

}

void MultiToolbarsStackedWidget::on_grayscaleButton_clicked()
{

}

void MultiToolbarsStackedWidget::on_speedComboBox_currentIndexChanged(QString )
{

}

void MultiToolbarsStackedWidget::on_magneticButton_clicked()
{

}

void MultiToolbarsStackedWidget::on_bookmarksButton_clicked()
{

}
