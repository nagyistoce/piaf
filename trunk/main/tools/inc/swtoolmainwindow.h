#ifndef SWTOOLMAINWINDOW_H
#define SWTOOLMAINWINDOW_H

#include <QMainWindow>

namespace Ui {
    class SwToolMainWindow;
}

class SwToolMainWindow : public QMainWindow {
    Q_OBJECT
public:
    SwToolMainWindow(QWidget *parent = 0);
    ~SwToolMainWindow();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::SwToolMainWindow *ui;
};

#endif // SWTOOLMAINWINDOW_H
