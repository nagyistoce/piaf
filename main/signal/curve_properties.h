/****************************************************************************
** Form interface generated from reading ui file 'curve_properties.ui'
**
** Created: mar jan 7 17:30:00 2003
**      by:  The User Interface Compiler (uic)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/
#ifndef PROPERTIESFORM_H
#define PROPERTIESFORM_H

#include <qvariant.h>
#include <qdialog.h>
#include <qcolordialog.h>

#include "qwt_ext_plot.h"

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

class PropertiesForm : public QDialog
{
    Q_OBJECT

public:
    PropertiesForm( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );    ~PropertiesForm();

    QGroupBox* plotGroupBox;
    QGroupBox* penGroupBox;
    QGroupBox* validGroupBox;
    QGroupBox* GroupBox4;
    QLabel* curve_Style;
    QLabel* Name;
    QLabel* pen_Style;
    QLabel* Width;
    QLabel* plot_Style;
    QComboBox* curveComboBox;
    QComboBox* penComboBox;
    QComboBox* plotComboBox;
    QLineEdit* NameEdit;
    QPushButton* dotsColorButton;
    QPushButton* ColorButton;
    QPushButton* CancelButton;
    QPushButton* OKButton;
    QSpinBox* widthSpinBox;

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
    QPen *pen;
    QPen myPen;
    
    
    QString new_name;
    QString *name;
    double x[50];
    double y[50];
    long curveID;

    int *style;
    int myStyle;
    
    QwtExtPlot *myPlot;
    QwtSymbol *crv_symbol;
    QwtSymbol my_symbol;
    
    QColorDialog *select;
};

#endif // PROPERTIESFORM_H

