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

private slots:
	void on_bookmarksButton_clicked();
	void on_magneticButton_clicked();
	void on_speedComboBox_currentIndexChanged(QString );
	void on_grayscaleButton_clicked();
	void on_goLastButton_clicked();
	void on_goNextButton_clicked();
	void on_playButton_toggled(bool checked);
	void on_goPrevButton_clicked();
	void on_goFirstButton_clicked();
};

#endif // MULTITOOLBARSSTACKEDWIDGET_H
