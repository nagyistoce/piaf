/****************************************************************************
** Form interface generated from reading ui file 'ui/marker_dialog.ui'
**
** Created: Wed Feb 24 17:22:08 2010
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#ifndef MARKERDIALOGFORM_H
#define MARKERDIALOGFORM_H

#include <qvariant.h>
#include <qpixmap.h>
#include <qdialog.h>
#include "qcolordialog.h"
#include "qwt_ext_plot.h"
#include "qlistboxmarker.h"

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QSpacerItem;
class QGroupBox;
class QLabel;
class QLineEdit;
class QListBox;
class QListBoxItem;
class QComboBox;
class QPushButton;
class QSpinBox;

class MarkerDialogForm : public QDialog
{
    Q_OBJECT

public:
    MarkerDialogForm( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
    ~MarkerDialogForm();

    QGroupBox* GroupBox2;
    QLabel* xposTextLabel;
    QLabel* yposTextLabel;
    QLineEdit* xposEdit;
    QLineEdit* yposEdit;
    QListBox* ListBox;
    QGroupBox* GroupBox5;
    QComboBox* symbolComboBox;
    QPushButton* PushButton6;
    QLabel* mrkColorLabel_2;
    QPushButton* newPushButton;
    QPushButton* deletePushButton;
    QPushButton* cancelPushButton;
    QGroupBox* GroupBox1;
    QLabel* TextLabel5;
    QLabel* modeTextLabel;
    QLineEdit* NameEdit;
    QLabel* nameLabel;
    QSpinBox* widthSpinBox;
    QComboBox* styleComboBox;
    QComboBox* modeComboBox;
    QLabel* widthLabel;
    QLabel* mrkColorLabel;
    QPushButton* colorPushButton;
    QPushButton* applyPushButton;

public slots:
    virtual void init( QwtExtPlot * plot );
    virtual void destroy();
    virtual void newMarker();
    virtual void removeMarker();
    virtual void itemSelected( QListBoxItem * pQlbi );
    virtual void setMarkerLabel();
    virtual void set_MarkerPos( double x, double y );
    virtual void setMarkerXPos();
    virtual void setMarkerYPos();
    virtual void setMarkerWidth();
    virtual void setMarkerColor();
    virtual void setMarkerMode( int value );
    virtual void setMarkerStyle( int value );
    virtual void setSymbolColor();
    virtual void setMarkerSymbol( int value );
    virtual void applyProperties();
    virtual void closeMarkerDialog();
    virtual void purchaseStyle( long key, int & mode, int & style, int & symbol );

signals:
    void set_marker_pos();

protected:
    QwtSymbol default_symbol;
    QPen symbol_pen;
    QwtExtPlot *my_plot;
    QPen mrk_pen;
    QwtSymbol my_symbol;
    QArray <long> mrk_keys;
    double xpos;
    double ypos;
    QColorDialog * select;
    QwtMarker::LineStyle line_style;
    QListBoxMarker * pCurrLbm;


protected slots:
    virtual void languageChange();

private:
    QPixmap image0;
    QPixmap image1;
    QPixmap image2;
    QPixmap image3;
    QPixmap image4;
    QPixmap image5;
    QPixmap image6;
    QPixmap image7;
    QPixmap image8;
    QPixmap image9;

};

#endif // MARKERDIALOGFORM_H
