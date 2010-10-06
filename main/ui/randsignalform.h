/****************************************************************************
** Form interface generated from reading ui file 'ui/randsignalform.ui'
**
** Created: Wed Feb 24 17:22:08 2010
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#ifndef RANDSIGNALFORM_H
#define RANDSIGNALFORM_H

#include <qvariant.h>
#include <qdialog.h>
#include "qwt_ext_plot.h"
#include "workshopmeasure.h"

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QSpacerItem;
class QLabel;
class QLineEdit;
class QPushButton;
class QComboBox;

class RandSignalForm : public QDialog
{
    Q_OBJECT

public:
    RandSignalForm( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
    ~RandSignalForm();

    QLabel* textLabel1;
    QLabel* textLabel1_3;
    QLabel* textLabel1_2;
    QLineEdit* paramEdit2;
    QLineEdit* paramEdit3;
    QLabel* paramLabel3;
    QLabel* paramLabel2;
    QLabel* paramLabel1;
    QPushButton* Close_Button;
    QLineEdit* samplesEdit;
    QComboBox* signalTypeCombo;
    QLineEdit* paramEdit1;
    QLineEdit* nameEdit;
    QPushButton* CreateButton;
    QPushButton* TestButton;

    WorkshopMeasure * Measure();

public slots:
    virtual void Close_Button_clicked();
    virtual void signalTypeCombo_textChanged( int value );
    virtual void TestButton_released();
    virtual void CreateButton_released();

signals:
    void newMeasure(WorkshopMeasure *);

protected:

protected slots:
    virtual void languageChange();

private:
    WorkshopMeasure myMeasure;
    QwtExtPlot *myPlot;
    long curveID;
    QValueVector<double> xvals;

    void init();
    void destroy();

};

#endif // RANDSIGNALFORM_H
