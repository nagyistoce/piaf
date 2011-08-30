#ifndef MULTITOOLBARSSTACKEDWIDGET_H
#define MULTITOOLBARSSTACKEDWIDGET_H

#include <QStackedWidget>

namespace Ui {
    class MultiToolbarsStackedWidget;
}

class MultiToolbarsStackedWidget : public QStackedWidget
{
    Q_OBJECT

public:
    explicit MultiToolbarsStackedWidget(QWidget *parent = 0);
    ~MultiToolbarsStackedWidget();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::MultiToolbarsStackedWidget *ui;
};

#endif // MULTITOOLBARSSTACKEDWIDGET_H
