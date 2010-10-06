/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/
#define SINUS             	0
#define SQUARE            1
#define WHITE_NOISE  	2
#define EXAMPLE         	3

#include <stdlib.h>

void RandSignalForm::init()
{
    // Initialisation of the plot area
    myPlot = new QwtExtPlot(this, "local view plot");
    myPlot->setGeometry( QRect( 20, 205, 550, 300 ) );
    myPlot->setTitle("Signal Preview");
    myPlot->setAutoLegend(TRUE);

    myPlot->set_plotMode(XY_MODE);
    myPlot->show();
    curveID = myPlot->insertCurve("Preview Curve");
}

void RandSignalForm::destroy()
{
    delete myPlot;
}

void RandSignalForm::Close_Button_clicked()
{
    close();
}

void RandSignalForm::signalTypeCombo_textChanged( int value)
{
    switch(value) 
    {
    case SINUS:
	paramLabel1->setText("Gain");
	if(paramLabel1->isHidden())
	{
	    paramLabel1->show();
	    paramEdit1->show();
	}
	paramLabel2->setText("w");
	if(paramLabel2->isHidden())
	{
	    paramLabel2->show();
	    paramEdit2->show();
	}
	paramLabel3->setText("Phi");
	if(paramLabel3->isHidden())
	{
	    paramLabel3->show();
	    paramEdit3->show();
	}
	break;
    case SQUARE:
	paramLabel1->setText("Gain");
	if(paramLabel1->isHidden())
	{
	    paramLabel1->show();
	    paramEdit1->show();
	}
	paramLabel2->setText("width A");
	if(paramLabel2->isHidden())
	{
	    paramLabel2->show();
	    paramEdit2->show();
	}
	paramLabel3->setText("width B");
	if(paramLabel3->isHidden())
	{
	    paramLabel3->show();
	    paramEdit3->show();
	}
	break;
    case WHITE_NOISE:
	paramLabel1->setText("Gain");
	if(paramLabel1->isHidden())
	{
	    paramLabel1->show();
	    paramEdit1->show();
	}
	if(!paramLabel2->isHidden())
	{
	    paramLabel2->hide();
	    paramEdit2->hide();
	}
	if(!paramLabel3->isHidden())
	{
	    paramLabel3->hide();
	    paramEdit3->hide();
	}
	break;
    case EXAMPLE:
	paramLabel1->setText("Gain");
	if(paramLabel1->isHidden())
	{
	    paramLabel1->show();
	    paramEdit1->show();
	}
	if(!paramLabel2->isHidden())
	{
	    paramLabel2->hide();
	    paramEdit2->hide();
	}
	if(!paramLabel3->isHidden())
	{
	    paramLabel3->hide();
	    paramEdit3->hide();
	}
	break;
    }
}

void RandSignalForm::TestButton_released()
{
    int newsize = samplesEdit->text().toInt();
    QString newname = nameEdit->text();
    int signalType = signalTypeCombo->currentItem();
    double k, w, phi;
    unsigned int wa, wb, wc;
    double value;
    double phase;
    
    // init a new measure for preview
    myMeasure.unvalidate();
    myMeasure.setLabel(newname);
    myMeasure.clear();
    myMeasure.resize(newsize);
    xvals.clear();
    xvals.resize(newsize);
    
    switch(signalType)
    {
    case SINUS:
	k = paramEdit1->text().toDouble();
	w = paramEdit2->text().toDouble();
	phi = paramEdit3->text().toDouble();
	
	for(int i=0; i<newsize; i++)
	{
	    myMeasure[i] = k*sin(w*i+phi);
	    xvals[i] = (double)i;
	}   
	break;
    case SQUARE:
	k = paramEdit1->text().toDouble();
	wa = paramEdit2->text().toUInt();
	wb = paramEdit3->text().toUInt();
	wc=0;
	value = k;
	for(int i=0; i<newsize; i++)
	{
	    myMeasure[i] = value;
	    xvals[i] = (double)i;
	    wc++;
	    if((value==k)&&(wc>=wa))
	    {
		value = k*-1.;
		wc=0;
	    }
	    if((value==k*-1.)&&(wc>=wb))
	    {
		value = k;
		wc=0;
	    }
	}   
	break;
    case WHITE_NOISE:
	break;
    case EXAMPLE:
	phase = 0.0;
	k = paramEdit1->text().toDouble();
	for(int i=0; i<newsize; i++)
	{
	    if (phase > (M_PI - 0.0001)) phase = 0;
	    myMeasure[i] = k*sin(phase) * (-1.0 + 2.0 * double(rand()) / double(RAND_MAX));
	    xvals[i] = (double)i;	    
	    phase += M_PI * 0.02;
	}
	break;
    }
    myPlot->setCurveRawData(curveID, &xvals[0], &myMeasure[0], newsize);
    myPlot->setCurveTitle(curveID, newname);
    myPlot->replot();
}

WorkshopMeasure * RandSignalForm::Measure()
{
    return &myMeasure;
}

void RandSignalForm::CreateButton_released()
{
    // Validation of the variable myMeasure
    myMeasure.setShortLabel("virtual");
    myMeasure.actuateDate();
    myMeasure.validate();
    
    emit newMeasure(&myMeasure);
}



