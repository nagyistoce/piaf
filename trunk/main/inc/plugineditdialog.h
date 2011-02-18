#ifndef PLUGINEDITDIALOG_H
#define PLUGINEDITDIALOG_H

#include <QDialog>

namespace Ui {
    class PluginEditDialog;
}

class PluginEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PluginEditDialog(QWidget *parent = 0);
    ~PluginEditDialog();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::PluginEditDialog *ui;
	QString lastPluginDir;
private slots:
	void on_saveButton_clicked();
 void on_delButton_clicked();
	void on_iconButton_clicked();
	void on_addButton_clicked();
};

#endif // PLUGINEDITDIALOG_H
