#ifndef MOVIEBOOKMARKFORM_H
#define MOVIEBOOKMARKFORM_H

#include <QWidget>
#include "videoplayertool.h"

namespace Ui {
    class MovieBookmarkForm;
}

class MovieBookmarkForm : public QWidget
{
    Q_OBJECT

public:
    explicit MovieBookmarkForm(QWidget *parent = 0);
    ~MovieBookmarkForm();

	void setBookmarkList(QList<video_bookmark_t> list);
protected:
    void changeEvent(QEvent *e);

private:
    Ui::MovieBookmarkForm *ui;
	QList<video_bookmark_t> m_listBookmarks;
signals:
	void signalNewBookmarkList(QList<video_bookmark_t>);

private slots:
	void on_delButton_clicked();
};

#endif // MOVIEBOOKMARKFORM_H
