#include "moviebookmarkform.h"
#include "ui_moviebookmarkform.h"

MovieBookmarkForm::MovieBookmarkForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MovieBookmarkForm)
{
    ui->setupUi(this);

	// change icon
	QPixmap winIcon = QPixmap(BASE_DIRECTORY "images/pixmaps/IconBookmark.png");
	setIcon(winIcon);

	//

}

MovieBookmarkForm::~MovieBookmarkForm()
{
    delete ui;
}

void MovieBookmarkForm::setBookmarkList(QList<video_bookmark_t> list) {
	m_listBookmarks = list;
	ui->treeWidget->clear();

	QList<video_bookmark_t>::iterator it;
	for(it = m_listBookmarks.begin(); it!=m_listBookmarks.end(); it++) {
		// append tree widget item
		video_bookmark_t mk = *it;
		QStringList itemList;
		QString str;
		str.sprintf("%d", mk.index);
		itemList.append(str);

		str.sprintf("%d %%", mk.percent);
		itemList.append(str);

		//QTreeWidgetItem * newItem =
				new QTreeWidgetItem(ui->treeWidget, itemList);
	}
}

void MovieBookmarkForm::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MovieBookmarkForm::on_delButton_clicked()
{
	// Del selected items
	//QTreeWidget::iter
	QList<QTreeWidgetItem *> selList = ui->treeWidget->selectedItems () ;
	if(selList.isEmpty()) return;

	QList<QTreeWidgetItem *>::iterator selItem;
	for(selItem = selList.begin(); selItem!=selList.end(); selItem++) {
		QTreeWidgetItem * item = *selItem;

		// get index
		int idx = ui->treeWidget->indexOfTopLevelItem(item);

		// delete from list
		m_listBookmarks.removeAt(idx);
	}

	// update
	setBookmarkList(m_listBookmarks);

	emit signalNewBookmarkList(m_listBookmarks);
}

void MovieBookmarkForm::on_exportButton_clicked()
{
	// \todo : launch export window

	emit signalExportSequence(m_listBookmarks);

}
