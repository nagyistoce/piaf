#ifndef VIDACQSETTINGSWINDOW_H
#define VIDACQSETTINGSWINDOW_H

#include <QMainWindow>

namespace Ui {
    class VidAcqSettingsWindow;
}

class VidAcqSettingsWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit VidAcqSettingsWindow(QWidget *parent = 0);
    ~VidAcqSettingsWindow();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::VidAcqSettingsWindow *ui;
};

#endif // VIDACQSETTINGSWINDOW_H
