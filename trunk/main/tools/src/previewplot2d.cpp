/***************************************************************************
      previewplot2d.cpp  -  2D plotting preview for Piaf project
                             -------------------
    begin                : July 2002
    copyright            : (C) 2002-2003 by Olivier VINE, Christophe SEYVE - SISELL
    email                : olivier.vine@sisell.com / christophe.seyve@sisell.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "previewplot2d.h"

// pixmaps includes
#include "iconSelection.xpm"
#include "IconMoveView.xpm"
#include "iconZoomRec.xpm"
#include "iconZoomVert.xpm"
#include "iconZoomHor.xpm"
#include "iconZoomEll.xpm"
#include "iconAutoX.xpm"
#include "iconAutoY.xpm"
//Added by qt3to4:
#include <Q3Frame>
#include <QPixmap>
#include <QMouseEvent>
#include <Q3ActionGroup>

PreviewPlot2D::PreviewPlot2D(QWidget* parent, const char *name, Qt::WidgetAttribute wflags):
        WorkshopTool(parent, name, wflags)
{
	pWin->setCaption(name);
	view_back = new Q3HBox(pWin);
	view_back->setFrameStyle( Q3Frame::StyledPanel | Q3Frame::Sunken );

	initTool();
	initPlot();
	initVars();
	//initActions();
	//initToolBar();
	
	connect(this, SIGNAL(newMeasure(int)), SLOT(onNewMeasure(int)));
	//connect(this, SIGNAL(rmMeasure(int)), SLOT(onDelMeasure(int)));
	pWin->show();
}

PreviewPlot2D::PreviewPlot2D(WorkshopMeasure* pDoc, QWidget* parent, const char *name,
							 Qt::WidgetAttribute wflags):
        WorkshopTool(parent, name, wflags)
{
	pWin->setCaption(name);

	view_back = new Q3HBox(pWin);
	view_back->setFrameStyle( Q3Frame::StyledPanel | Q3Frame::Sunken );
	
	initTool();
	initPlot();
	initVars();
	initActions();
	initToolBar();

	connect(this, SIGNAL(newMeasure(int)), SLOT(onNewMeasure(int)));
	//connect(this, SIGNAL(rmMeasure(int)), SLOT(onDelMeasure(int)));

	addNewMeasure(pDoc);
	pWin->show();
}

void PreviewPlot2D::initTool()
{
	// enable displaying measures
	enableDisplay(CLASS_MEASURE);
	// disable all ohter actions
	disableCreate(CLASS_MEASURE);
	disableDisplay(CLASS_IMAGE);
	disableCreate(CLASS_IMAGE);
	disableDisplay(CLASS_VIDEO);
	disableCreate(CLASS_VIDEO);
}

void PreviewPlot2D::initPlot()
{
	pPlot = new QwtExtPlot(view_back, "the plot");
 	pWin->setCentralWidget(view_back);
 	pWin->setFixedSize(300, 200);
 	pPlot->setFixedSize(300, 200);

	// set plot mode
	pPlot->set_plotMode(GRAPH_MODE);
	// outline
//FIXME	pPlot->enableOutline(TRUE);
//FIXME    pPlot->setOutlinePen(red);

	connect(pPlot, SIGNAL(plotMouseMoved(const QMouseEvent&)),
            SLOT(plotMouseMoved( const QMouseEvent&)));
    connect(pPlot, SIGNAL(plotMousePressed(const QMouseEvent &)),
            SLOT(plotMousePressed( const QMouseEvent&)));
    connect(pPlot, SIGNAL(plotMouseReleased(const QMouseEvent &)),
            SLOT(plotMouseReleased( const QMouseEvent&)));
}

void PreviewPlot2D::initVars()
{
	currKey = -1;
	autoscaleX = true;
	autoscaleY = true;
	zoomMode = NO_ZOOM;	
}

void PreviewPlot2D::initActions()
{
	QIcon iconSelect, iconMove;
	QIcon iconZoomV, iconZoomH, iconZoomR, iconZoomE;
	QIcon iconAutoScaleX, iconAutoScaleY;
	
	iconSelect = QIcon(iconSelection);
	iconMove = QIcon(IconMoveView_xpm);
	iconZoomR = QIcon(iconZoomRec);
	iconZoomV = QIcon(iconZoomVert);
	iconZoomH = QIcon(iconZoomHor);
	iconZoomE = QIcon(iconZoomEll);
	iconAutoScaleX = QIcon(iconAutoX);
	iconAutoScaleY = QIcon(iconAutoY);

	// creating group of zoom actions
	grpZoom = new QActionGroup( this );
	grpZoom->setExclusive( TRUE );
	connect( grpZoom, SIGNAL(selected(QAction*)), this, SLOT(slotZoom(QAction*)) );
	
	Select = new QAction(iconSelect, tr("Selection tool"), tr("&Selection"), grpZoom, "select");
	Select->setStatusTip(tr("Select a curve"));
	Select->setWhatsThis(tr("You can select on ore many curves !"));
	Select->setToggleAction (true);

	Move = new QAction(iconMove, tr("Moving plot"), tr("&Move"), grpZoom, "move");
	Move->setStatusTip(tr("Dragging the curves on plot"));
	Move->setWhatsThis(tr("You can grag the curves !"));
	Move->setToggleAction (true);

	ZoomR = new QAction(iconZoomR, tr("Rectangular zoom"), tr("&Rectangular"), grpZoom, "rect");
	ZoomR->setStatusTip(tr("Zoom on a rectangular area"));
	ZoomR->setWhatsThis(tr("Designate a rectangular area and zoom !"));
	ZoomR->setToggleAction (true);

	ZoomV = new QAction(iconZoomV,tr("Vertical zoom"), tr("&Vertical"), grpZoom, "vert");
	ZoomV->setStatusTip(tr("Zoom on a vertical area"));
	ZoomV->setWhatsThis(tr("Designate a vertical area and zoom !"));
	ZoomV->setToggleAction (true);
	
	ZoomH = new QAction(iconZoomH, tr("Horizontal zoom"), tr("&Horizontal"), grpZoom, "horiz");
	ZoomH->setStatusTip(tr("Zoom on a horizontal area"));
	ZoomH->setWhatsThis(tr("Designate a horizontal area and zoom !"));
	ZoomH->setToggleAction (true);
	
	ZoomE = new QAction(iconZoomE, tr("Elliptic zoom"), tr("&Elliptic"), grpZoom, "ellip");
	ZoomE->setStatusTip(tr("Zoom on an elliptic area"));
	ZoomE->setWhatsThis(tr("Designate an Elliptic area and zoom !"));
	ZoomE->setToggleAction (true);

	AutoScaleX = new QAction(iconAutoScaleX, tr("Autoscale X"), tr("&Autoscale X"), this, "auto x");
	AutoScaleX->setStatusTip(tr("Autoscale on X axe"));
	AutoScaleX->setWhatsThis(tr("Autoscale on X axe"));
	AutoScaleX->setToggleAction (true);
	AutoScaleX->setOn (true);
	connect(AutoScaleX, SIGNAL(toggled(bool)), this, SLOT(slotAutoX(bool)));

	AutoScaleY = new QAction(iconAutoScaleY, tr("Autoscale Y"), tr("&Autoscale Y"), this, "auto y");
	AutoScaleY->setStatusTip(tr("Autoscale on Y axe"));
	AutoScaleY->setWhatsThis(tr("Autoscale on Y axe"));
	AutoScaleY->setToggleAction (true);
	AutoScaleY->setOn (true);
	connect(AutoScaleY, SIGNAL(toggled(bool)), this, SLOT(slotAutoY(bool)));
}


void PreviewPlot2D::initToolBar()
{
	// creating toolbar
	zoomToolBar = new Q3ToolBar(pWin, "zoom operations");
	Select->addTo(zoomToolBar);
	zoomToolBar->addSeparator();	
	ZoomR->addTo(zoomToolBar);
	ZoomV->addTo(zoomToolBar);
	ZoomH->addTo(zoomToolBar);
	ZoomE->addTo(zoomToolBar);
	zoomToolBar->addSeparator();
	Move->addTo(zoomToolBar);
	zoomToolBar->addSeparator();
	AutoScaleX->addTo(zoomToolBar);
	AutoScaleY->addTo(zoomToolBar);
}

// SLOTS
void PreviewPlot2D::onNewMeasure(int ptr)
{
	WorkshopMeasure *pWm = (WorkshopMeasure *)ptr;
	if(currKey != -1)
	{
		// remove current curve
		pPlot->removeCurve(currKey);
	}
	pPlot->set_bufferSize(pWm->count());
	currKey = pPlot->insertCurve(pWm->getLabel());
	for(unsigned int i=0; i<pWm->count(); i++)
		pPlot->putValue(currKey, (*pWm)[i]);

	pPlot->replot();
}

void PreviewPlot2D::plotMouseMoved( const QMouseEvent& e)
{
#if 0
	if(zoomMode==MOV_ZOOM)
	{
		double dx = pPlot->invTransform(QwtPlot::xBottom, e.pos().x()) - pPlot->invTransform(QwtPlot::xBottom, memPoint.x());
		double dy = pPlot->invTransform(QwtPlot::yLeft, e.pos().y()) - pPlot->invTransform(QwtPlot::yLeft, memPoint.y());
		double xmin = pPlot->axisScale(QwtPlot::xBottom)->lBound() - dx;
		double xmax = pPlot->axisScale(QwtPlot::xBottom)->hBound() - dx;
		double ymin = pPlot->axisScale(QwtPlot::yLeft)->lBound() - dy;
		double ymax = pPlot->axisScale(QwtPlot::yLeft)->hBound() - dy;
		pPlot->setAxisScale(QwtPlot::xBottom, xmin, xmax);
		pPlot->setAxisScale(QwtPlot::yLeft, ymin, ymax);
		memPoint = e.pos();
		pPlot->replot();
		
	}
#endif
}

void PreviewPlot2D::plotMousePressed( const QMouseEvent& e)
{
#if 0
	if(e.button()==Qt::LeftButton)
	{
		// store position
		memPoint = e.pos();
 
		switch(zoomMode)
		{
			default:
			case NO_ZOOM:
		        pPlot->setOutlineStyle(Qwt::Cross);
				break;
			case V_ZOOM:
				zoomMarkerKey = pPlot->insertMarker();
				pPlot->setMarkerPen(zoomMarkerKey, QPen(red));
				pPlot->setMarkerLineStyle(zoomMarkerKey, QwtPlotMarker::VLine);
		        pPlot->setOutlineStyle(Qwt::VLine);
				pPlot->replot();
				break;
			case H_ZOOM:
				zoomMarkerKey = pPlot->insertMarker();
				pPlot->setMarkerPen(zoomMarkerKey, QPen(red));
				pPlot->setMarkerLineStyle(zoomMarkerKey, QwtPlotMarker::HLine);
		        pPlot->setOutlineStyle(Qwt::HLine);
				pPlot->replot();
				break;			
			case REC_ZOOM:
		        pPlot->setOutlineStyle(Qwt::Rect);
				break;
			case ELL_ZOOM:
		        pPlot->setOutlineStyle(Qwt::Ellipse);
				break;
		}
	}
#endif
}

void PreviewPlot2D::plotMouseReleased( const QMouseEvent& e)
{	
#if 0
	if(e.button()==Qt::LeftButton)
	{
		switch(zoomMode)
		{
		case NO_ZOOM:
			break;
		case ELL_ZOOM:
		case REC_ZOOM:
			onZoomRec(e);
			break;
		case H_ZOOM:
			pPlot->removeMarker(zoomMarkerKey);
			onHZoom(e);
			break;
		case V_ZOOM:
			pPlot->removeMarker(zoomMarkerKey);
			onVZoom(e);
			break;
		}
		pPlot->replot();
	}
#endif
}

void PreviewPlot2D::slotZoom(QAction* onAction)
{
	if(onAction==Select)
		zoomMode = NO_ZOOM;		
	else if(onAction==Move)
		zoomMode = MOV_ZOOM;		
	else if(onAction==ZoomR)
		zoomMode = REC_ZOOM;
	else if(onAction==ZoomV)
		zoomMode = V_ZOOM;
	else if(onAction==ZoomH)
		zoomMode = H_ZOOM;
	else if(onAction==ZoomE)
		zoomMode = ELL_ZOOM;
	else
		zoomMode = NO_ZOOM;
}

void PreviewPlot2D::slotAutoX(bool on)
{
	autoscaleX = on;
	if(on)
		pPlot->setAxisAutoScale(QwtPlot::xBottom);
	
	pPlot->replot();
}

void PreviewPlot2D::slotAutoY(bool on)
{
	autoscaleY = on;
	
	if(on)
		pPlot->setAxisAutoScale(QwtPlot::yLeft);	

	pPlot->replot();	
}

void PreviewPlot2D::onZoomRec(const QMouseEvent& e)
{
    // some shortcuts
    int axl= QwtPlot::yLeft, axb= QwtPlot::xBottom;
 
	// Don't invert any scales which aren't inverted
	int x1 = qwtMin(memPoint.x(), e.pos().x());
    int x2 = qwtMax(memPoint.x(), e.pos().x());
    int y1 = qwtMin(memPoint.y(), e.pos().y());
    int y2 = qwtMax(memPoint.y(), e.pos().y());
        
	// limit selected area to a minimum of 11x11 points
	int lim = 5 - (y2 - y1) / 2;
	if (lim > 0)
	{
		y1 -= lim;
		y2 += lim;
	}
	lim = 5 - (x2 - x1 + 1) / 2;
	if (lim > 0)
	{
		x1 -= lim;
		x2 += lim;
	}
        
	// Set fixed scales
	if(!autoscaleY)
		pPlot->setAxisScale(axl, pPlot->invTransform(axl,y1), pPlot->invTransform(axl,y2));
	if(!autoscaleX)
		pPlot->setAxisScale(axb, pPlot->invTransform(axb,x1), pPlot->invTransform(axb,x2));
	pPlot->replot();
}

void PreviewPlot2D::onHZoom(const QMouseEvent& e)
{
    // some shortcuts
    int axl= QwtPlot::yLeft;
 
	// Don't invert any scales which aren't inverted
    int y1 = qwtMin(memPoint.y(), e.pos().y());
    int y2 = qwtMax(memPoint.y(), e.pos().y());
        
	// limit selected area to a minimum of 11x11 points
	int lim = 5 - (y2 - y1) / 2;
	if (lim > 0)
	{
		y1 -= lim;
		y2 += lim;
	}
       
	// Set fixed scales
	if(!autoscaleY)
		pPlot->setAxisScale(axl, pPlot->invTransform(axl,y1), pPlot->invTransform(axl,y2));
	pPlot->replot();
}

void PreviewPlot2D::onVZoom(const QMouseEvent& e)
{
    // some shortcuts
    int axb= QwtPlot::xBottom;
 
	// Don't invert any scales which aren't inverted
	int x1 = qwtMin(memPoint.x(), e.pos().x());
    int x2 = qwtMax(memPoint.x(), e.pos().x());
        
	// limit selected area to a minimum of 11x11 points
	int lim = 5 - (x2 - x1 + 1) / 2;
	if (lim > 0)
	{
		x1 -= lim;
		x2 += lim;
	}
        
	// Set fixed scales
	if(!autoscaleX)
		pPlot->setAxisScale(axb, pPlot->invTransform(axb,x1), pPlot->invTransform(axb,x2));
	pPlot->replot();
}
