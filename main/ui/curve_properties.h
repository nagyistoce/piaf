/****************************************************************************
** Form interface generated from reading ui file 'ui/curve_properties.ui'
**
** Created: Wed Feb 24 17:22:08 2010
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#ifndef PROPERTIESFORM_H
#define PROPERTIESFORM_H

#include <qvariant.h>
#include <qpixmap.h>
#include <qdialog.h>
#include "qwt_ext_plot.h"
#include "qcolordialog.h"

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QSpacerItem;
class QGroupBox;
class QLabel;
class QComboBox;
class QLineEdit;
class QPushButton;
class QSpinBox;

class PropertiesForm : public QDialog
{
    Q_OBJECT

public:
    PropertiesForm( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
    ~PropertiesForm();

    QGroupBox* plotGroupBox;
    QLabel* curve_Style;
    QLabel* Name;
    QComboBox* curveComboBox;
    QLineEdit* NameEdit;
    QGroupBox* GroupBox4;
    QLabel* plot_Style;
    QPushButton* dotsColorButton;
    QComboBox* plotComboBox;
    QGroupBox* penGroupBox;
    QLabel* pen_Style;
    QLabel* Width;
    QComboBox* penComboBox;
    QPushButton* ColorButton;
    QSpinBox* widthSpinBox;
    QGroupBox* validGroupBox;
    QPushButton* CancelButton;
    QPushButton* OKButton;

public slots:
    virtual void init( const QString * last_name, QPen * last_pen, int * last_style, QwtSymbol * last_symbol );
    virtual void destroy();
    virtual void setCurveName();
    virtual void setColor();
    virtual void setDotsColor();
    virtual void setWidth();
    virtual void set_PenStyle( int value );
    virtual void set_CurveStyle( int value );
    virtual void drawMarker( int value );
    virtual void validProperties();

protected:
    QPen symbol_pen;
    QwtSymbol *crv_symbol;
    QColorDialog *select;
    QString new_name;
    double x[50];
    long curveID;
    int *style;
    QPen *pen;
    QwtExtPlot *myPlot;
    QPen myPen;
    QString *name;
    int myStyle;
    double y[50];
    QwtSymbol my_symbol;


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
    QPixmap image10;
    QPixmap image11;
    QPixmap image12;
    QPixmap image13;
    QPixmap image14;

};

#endif // PROPERTIESFORM_H
