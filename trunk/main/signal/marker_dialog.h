/***************************************************************************
                          marker_dialog.h  -  description
                             -------------------
    begin                : ven jan 10 2003
    copyright            : (C) 2003 by Paul Chatellier
    email                : paul@localhost.localdomain
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/****************************************************************************
** Form interface generated from reading ui file 'marker.ui'
**
** Created: ven jan 10 16:31:41 2003
**      by:  The User Interface Compiler (uic)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/
#ifndef MARKERDIALOGFORM_H
#define MARKERDIALOGFORM_H

#include <qvariant.h>
#include <qdialog.h>
#include <qcolordialog.h>
#include <qlistbox.h>

#include "qwt_ext_plot.h"
#include "qlistboxmarker.h"

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QListBox;
class QListBoxItem;
class QPushButton;
class QSpinBox;

class MarkerDialogForm : public QDialog
{
    Q_OBJECT

public:
    MarkerDialogForm( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
    ~MarkerDialogForm();

    QGroupBox* GroupBox1;
    QGroupBox* GroupBox2;
    QGroupBox* GroupBox5;
    QLabel* xposTextLabel;
    QLabel* yposTextLabel;
    QLabel* mrkColorLabel_2;
    QLabel* TextLabel5;
    QLabel* modeTextLabel;
    QLabel* nameLabel;
    QLabel* widthLabel;
    QLabel* mrkColorLabel;
    QLineEdit* NameEdit;
    QLineEdit* xposEdit;
    QLineEdit* yposEdit;
    QPushButton* deletePushButton;
    QPushButton* applyPushButton;
    QPushButton* newPushButton;
    QPushButton* closePushButton;
    QPushButton* PushButton6;
    QPushButton* colorPushButton;
    QListBox* ListBox;
    QComboBox* symbolComboBox;
    QComboBox* styleComboBox;
    QComboBox* modeComboBox;
    QSpinBox* widthSpinBox;

    
    
    
    

    void purchaseStyle( long key, int & mode, int & style, int & symbol );
    
signals:
    void set_marker_pos();

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

protected:
    QArray <long> mrk_keys;
    double xpos;
    double ypos;
            
    QwtExtPlot *my_plot;
    QwtSymbol default_symbol;
    QwtSymbol my_symbol;

    QPen mrk_pen;
    QPen symbol_pen;
    
    QColorDialog * select;
    QwtMarker::LineStyle line_style;

private:
    QListBoxMarker * pCurrLbm;
};

#endif // MARKERDIALOGFORM_H



