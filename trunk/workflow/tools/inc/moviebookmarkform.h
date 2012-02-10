#ifndef MOVIEBOOKMARKFORM_H
#define MOVIEBOOKMARKFORM_H

#include <QWidget>

#ifdef PIAF_LEGACY
#include "videoplayertool.h"
#include "workshopmovie.h"
#else
#include "workflowtypes.h"
#endif

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
	void signalExportSequence(QList<video_bookmark_t>);
private slots:
	void on_exportButton_clicked();
	void on_delButton_clicked();
};

#endif // MOVIEBOOKMARKFORM_H
