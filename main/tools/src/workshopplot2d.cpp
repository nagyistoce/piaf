/***************************************************************************
                          workshopPlot2D.cpp  -  description
                             -------------------
    begin                : ven nov 29 15:53:48 UTC 2002
    copyright            : (C) 2002 by Olivier Viné
    email                : olivier.vine@sisell.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <unistd.h>
#include "workshopplot2d.h"
//Added by qt3to4:
#include <Q3ValueList>
#include <QPixmap>
#include <Q3Frame>
#include <QMouseEvent>
#include <Q3ActionGroup>


WorkshopPlot2D::WorkshopPlot2D(QWidget* parent, const char *name, Qt::WidgetAttribute wflags):
        WorkshopTool(parent, name, wflags)
{
	
	pWorkspace=parent;
	view_back = new Q3HBox(pWin);
	view_back->setFrameStyle( Q3Frame::StyledPanel | Q3Frame::Sunken );
	
	initTool();
	
	initVars();

	//////////////
    initPlot();
    //////////////
	initActions();
	initToolBar();
	
	connect(this, SIGNAL(newMeasure(int)), SLOT(onNewMeasure(int)));
	connect(this, SIGNAL(rmMeasure(int)), SLOT(onDelMeasure(int)));
	filterManager = NULL;
	
	
	pWin->show();
}

WorkshopPlot2D::WorkshopPlot2D(WorkshopMeasure* pDoc, QWidget* parent,
							   const char *name, Qt::WidgetAttribute wflags):
        WorkshopTool(parent, name, wflags)
{
	pWin->setCaption(name);
	//pWorkspace=parent;

	view_back = new Q3HBox(pWin);
	view_back->setFrameStyle( Q3Frame::StyledPanel | Q3Frame::Sunken );
	
	pWorkspace=parent;
	initTool();
	
	initVars();
	///////////
	initPlot();
	///////////
	initActions();
	initToolBar();

	connect(this, SIGNAL(newMeasure(int)), SLOT(onNewMeasure(int)));
	connect(this, SIGNAL(rmMeasure(int)), SLOT(onDelMeasure(int)));

	addNewMeasure(pDoc);
	
	pWin->show();
}

WorkshopPlot2D::~WorkshopPlot2D()
{
	if(markerForm)
	   delete markerForm;
	if(pPlot)
		 delete pPlot;
	if(zoomToolBar)
		 delete zoomToolBar;
	if(properties)
		delete properties;
	if(my_cursor)
	    delete my_cursor;
}
	

void WorkshopPlot2D::initTool()
{
	// enable handling measures
	enableDisplay(CLASS_MEASURE);
	enableCreate(CLASS_MEASURE);
	// disable handling image and video
	disableDisplay(CLASS_IMAGE);
	disableCreate(CLASS_IMAGE);
	disableDisplay(CLASS_VIDEO);
	disableCreate(CLASS_VIDEO);
}

void WorkshopPlot2D::initPlot()
{
	
	pPlot = new QwtExtPlot(view_back, "the plot");
 	pWin->setCentralWidget(view_back);
	
	pPlot->setMinimumSize(400, 100);
	
	// set plot mode
	pPlot->set_plotMode(GRAPH_MODE);
	// outline
#if 0
	pPlot->enableOutline(TRUE);
    pPlot->setOutlinePen(red);
#endif
	my_cursor = new QwtExtCursor(pPlot);
	
	
	connect(pPlot, SIGNAL(plotMouseMoved(const QMouseEvent&)),
            SLOT(plotMouseMoved( const QMouseEvent&)));
    connect(pPlot, SIGNAL(plotMousePressed(const QMouseEvent &)),
            SLOT(plotMousePressed( const QMouseEvent&)));
    connect(pPlot, SIGNAL(plotMouseReleased(const QMouseEvent &)),
            SLOT(plotMouseReleased( const QMouseEvent&)));
			
	
}

void WorkshopPlot2D::initVars()
{
	pNewMeasure = NULL;	
 
	pPlot=NULL;
	// setting true for auto delete of HPenDict
	HPenDict.setAutoDelete(true);
	
	autoscaleX = false;
	autoscaleY = false;
	zoomMode = NO_ZOOM;	
	markerForm=NULL;
		
	properties=NULL;
	
	crv=0;
	my_cursor=NULL;
}

void WorkshopPlot2D::initActions()
{
	QPixmap iconSelect, iconMove;
	QPixmap iconZoomV, iconZoomH, iconZoomR, iconZoomE;
	QPixmap iconAutoScaleX, iconAutoScaleY;
	QPixmap iconMarkerForm, iconPropForm;
	chdir(BASE_DIRECTORY "images/pixmaps");
	
	iconSelect = QPixmap("IconSelCurve.xpm");
	iconMove = QPixmap("IconMoveView.xpm");
	iconZoomR = QPixmap("iconZoomRec.xpm");
	iconZoomV = QPixmap("iconZoomVert.xpm");
	iconZoomH = QPixmap("iconZoomHor.xpm");
	iconZoomE = QPixmap("iconZoomEll.xpm");
	iconAutoScaleX = QPixmap("iconAutoX.xpm");
	iconAutoScaleY = QPixmap("iconAutoY.xpm");
	iconMarkerForm = QPixmap("IconCurveMarker.xpm");
	iconPropForm = QPixmap("IconCurveProp.xpm");

	// creating group of zoom actions
	grpZoom = new Q3ActionGroup( this );
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

	ZoomV = new QAction(iconZoomV, tr("Vertical zoom"), tr("&Vertical"), grpZoom, "vert");
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

	AutoScaleX = new QAction(iconAutoScaleX, tr("Autoscale X"), tr("Autoscale &X"), this, "auto x");
	AutoScaleX->setStatusTip(tr("Autoscale on X axe"));
	AutoScaleX->setWhatsThis(tr("Autoscale on X axe"));
	AutoScaleX->setToggleAction (true);
	AutoScaleX->setOn (false);
	connect(AutoScaleX, SIGNAL(toggled(bool)), this, SLOT(slotAutoX(bool)));

	AutoScaleY = new QAction(iconAutoScaleY, tr("Autoscale Y"), tr("Autoscale &Y"), this, "auto y");
	AutoScaleY->setStatusTip(tr("Autoscale on Y axe"));
	AutoScaleY->setWhatsThis(tr("Autoscale on Y axe"));
	AutoScaleY->setToggleAction (true);
	AutoScaleY->setOn (false);
	connect(AutoScaleY, SIGNAL(toggled(bool)), this, SLOT(slotAutoY(bool)));
	
	/////////////////////////////////////
	MarkForm = new QAction(iconMarkerForm , tr("Marker Form"),tr("&Marker Form"), this, "marker");
	MarkForm->setStatusTip(tr("Set the Marker Form"));
	MarkForm->setWhatsThis(tr("Set the Marker Form"));
	//MarkForm->setToggleAction (true);
	MarkForm->setToggleAction (false);
	MarkForm->setOn (false);
	connect(MarkForm, SIGNAL(activated()), this, SLOT(slotMarkerForm()));
	
	PropForm = new QAction(iconPropForm, tr("Properties Form"), tr("&Properties Form"), this, "props");
	PropForm->setStatusTip(tr("Set the Properties Form"));
	PropForm->setWhatsThis(tr("Set the Properties Form"));
	PropForm->setToggleAction (false);
	PropForm->setOn (false);
	connect(PropForm, SIGNAL(activated()), this, SLOT(slotPropertiesForm()));
	
	CursorForm = new QAction(iconPropForm, tr("cursor form"), tr("&cursor form"), this, "cursor");
	CursorForm->setStatusTip(tr("Set a cursor which follows the curve"));
	CursorForm->setWhatsThis(tr("Set a cursor which follows the curve"));
	CursorForm->setToggleAction (true);


    // FILTERS
    QPixmap iconFilters = QPixmap(BASE_DIRECTORY "images/pixmaps/IconFilters.xpm");
	filtersAction = new QAction(iconFilters, tr("Filters Window"), tr("&Filters Window"), this, "filters");
	filtersAction->setStatusTip(tr("Launch filters window"));
	filtersAction->setWhatsThis(tr("Launch filters window"));
	filtersAction->setToggleAction (false);
	filtersAction->setOn (false);
	connect(filtersAction, SIGNAL(activated()), this, SLOT(slotFilters()));

}


void WorkshopPlot2D::slotMarkerForm()
{
	printf("toto!\n");
    //zoomMode = MARKER_FORM;		
	
	if(!markerForm)
	{
	    markerForm = new MarkerDialogForm( pPlot, "marker dialog" );
	}
	else
		printf("markerdialog already loaded!\n");
	
	markerForm->init(pPlot);
	if( my_cursor )
	    connect( my_cursor, SIGNAL( cursor_pos( double, double ) ), markerForm,
                                      SLOT( set_MarkerPos(double, double) ) );
	 markerForm->show();
}


////////////////////////////
void WorkshopPlot2D::slotPropertiesForm()
{
	QString newString;
	QPen newPen;
	int newStyle;
	QwtSymbol newSymbol;
	QwtPlotCurve * pQCurve;
	long crvKey;
	
	printf("SLOTPROPERTIESFORMS!\n");
    if(!highlightList.isEmpty())
	 {
		if(highlightList.count()==1)
		{
			if(!properties)
			{
	    		properties = new PropertiesForm( pPlot, "properties" );
			}
#if 0
			crvKey = *highlightList.begin();
			newString = pPlot->curveTitle( crvKey );
			newPen = pPlot->curvePen( crvKey );
			newStyle = pPlot->curveStyle( crvKey );
			newSymbol = pPlot->curveSymbol( crvKey );
			
		 	properties->init(&newString, &newPen, &newStyle, &newSymbol);
			properties->exec();
			Q3ValueList<long>::iterator it;
			for ( it = highlightList.begin(); it != highlightList.end(); ++it )
			{
				pQCurve = pPlot->curve(*it);
				pQCurve->setTitle(newString);
				pQCurve->setStyle(newStyle);
				pQCurve->setPen(newPen);
				pQCurve->setSymbol(newSymbol);
			}
#endif
		}
	/*	properties->init(&newString, &newPen, &newStyle, &newSymbol);
		properties->exec();
		QValueList<long>::iterator it;
		for ( it = highlightList.begin(); it != highlightList.end(); ++it )
		{
			pQCurve = pPlot->curve(*it);
			pQCurve->setTitle(newString);
			pQCurve->setStyle(newStyle);
			pQCurve->setPen(newPen);
			pQCurve->setSymbol(newSymbol);
		}*/
		
		pPlot->replot();
	}
	else
		printf("liste surbrillante vide!!\n");
}

///////////////////////////////////



void WorkshopPlot2D::initToolBar()
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
	MarkForm->addTo(zoomToolBar);
	PropForm->addTo(zoomToolBar);
	CursorForm->addTo(zoomToolBar);
}

void WorkshopPlot2D::clearHighlight()
{
	Q3IntDictIterator<QPen> it(HPenDict);
    for ( ; it.current(); ++it )
	{
#if 0
		pPlot->setCurvePen(it.currentKey(), *it.current());
#endif
	}
	
	highlightList.clear();	
	HPenDict.clear();
}

void WorkshopPlot2D::removeHighlight(long key)
{
#if 0
	pPlot->setCurvePen(key, *HPenDict[key]);
#endif
	highlightList.remove(key);	
	HPenDict.remove(key);
}

void WorkshopPlot2D::addHighlight(long key)
{
	highlightList.append(key);
#if 0
	QPen *pen = new QPen(pPlot->curvePen(key));
	HPenDict.insert(key, pen);
	QPen newpen (pPlot->curvePen(key));
	newpen.setColor(Qt::yellow);
	pPlot->setCurvePen(key, newpen);
#endif
}

bool WorkshopPlot2D::isHighlighted(long key)
{
	if(highlightList.isEmpty())
		return false;
	
	
	return (highlightList.find(key)!=highlightList.end());
}

// SLOTS
void WorkshopPlot2D::onNewMeasure(int ptr)
{
	WorkshopMeasure *pWm = (WorkshopMeasure *)ptr;
	pPlot->set_bufferSize(pWm->count());
	long id = pPlot->insertCurve(pWm->getLabel());
	for(unsigned int i=0; i<pWm->count(); i++)
		pPlot->putValue(id, (*pWm)[i]);

	pPlot->replot();
	curveDict.insert(id, pWm);
}

void WorkshopPlot2D::onDelMeasure(int ptr)
{
	WorkshopMeasure *pWm = (WorkshopMeasure *)ptr;
	long key = -1;
	Q3IntDictIterator<WorkshopMeasure> it(curveDict);
	for ( ; it.current(); ++it )
	{
		if(it.current()==pWm)
			key = it.currentKey();
	}
	if(key!=-1)
	{
		pPlot->removeCurve(key);
		curveDict.remove(key);
	}
	pPlot->replot();
}


/*
void MainWin::plotMouseMoved(const QMouseEvent &e)
{
    // Recupere les valeurs du point le plus proche et trace une croix (marqueur)
    if(pressed)
    {
      my_cursor->print( crv, e.pos().x(), e.pos().y(), xval, yval );
    }
}


void MainWin::plotMousePressed(const QMouseEvent &e)
{
    crv = my_plot->closestCurve( e.pos().x(), e.pos().y(), dist );

    // On met le flag pressed à TRUE
    if( e.button() == LeftButton )
        pressed = TRUE;
    else
        _popup->exec(QCursor::pos());
    
    // update cursor pos display
    plotMouseMoved(e);
    
    my_plot->enableOutline(FALSE);
}

 
void MainWin::plotMouseReleased(const QMouseEvent &e)
{
    pressed = FALSE;
}

*/
void WorkshopPlot2D::plotMouseMoved( const QMouseEvent& e)
{
	double dx, dy, xmin, xmax, ymin, ymax;
	
	
	switch(zoomMode)
	{
		default:
			break;
		
		case CURSOR_FOLLOW:
			if( crv != 0 )
				my_cursor->print( crv, e.pos().x(), e.pos().y(), xval, yval );
		    break;
			  
		case MOV_ZOOM:
	
			dx = pPlot->invTransform(QwtPlot::xBottom, e.pos().x()) - pPlot->invTransform(QwtPlot::xBottom, memPoint.x());
			dy = pPlot->invTransform(QwtPlot::yLeft, e.pos().y()) - pPlot->invTransform(QwtPlot::yLeft, memPoint.y());
#if 0
			xmin = pPlot->axisScale(QwtPlot::xBottom)->lBound() - dx;
			xmax = pPlot->axisScale(QwtPlot::xBottom)->hBound() - dx;
			ymin = pPlot->axisScale(QwtPlot::yLeft)->lBound() - dy;
			ymax = pPlot->axisScale(QwtPlot::yLeft)->hBound() - dy;
			pPlot->setAxisScale(QwtPlot::xBottom, xmin, xmax);
			pPlot->setAxisScale(QwtPlot::yLeft, ymin, ymax);
#endif
			memPoint = e.pos();
			pPlot->replot();
		    break;
	}
}


void WorkshopPlot2D::plotMousePressed( const QMouseEvent& e)
{ 
	int dist;
	
	
	if(e.button()==Qt::LeftButton)
	{
		// store position
		memPoint = e.pos();
 		
		switch(zoomMode)
		{
			default:
				break;
			
			case CURSOR_FOLLOW:
//if 0				 crv = pPlot->closestCurve( e.pos().x(), e.pos().y(), dist );
//if 0			     pPlot->enableOutline(FALSE);
			     break;
			case SELECT:
				 //if 0pPlot->enableOutline(FALSE);
				 break;	
			
			case MOV_ZOOM:
//if 0				pPlot->enableOutline(TRUE);
//if 0			    pPlot->setOutlineStyle(Qwt::Cross);
			
			case NO_ZOOM:
//if 0				pPlot->enableOutline(TRUE);
//if 0		        pPlot->setOutlineStyle(Qwt::Cross);
				break;
			
			case V_ZOOM:
//if 0				pPlot->enableOutline(TRUE);
//if 0				zoomMarkerKey = pPlot->insertMarker();
//if 0				pPlot->setMarkerPen(zoomMarkerKey, QPen(red));
//if 0				pPlot->setMarkerLineStyle(zoomMarkerKey, QwtMarker::VLine);
//if 0		        pPlot->setOutlineStyle(Qwt::VLine);
				pPlot->replot();
				break;
			
			case H_ZOOM:
//if 0				pPlot->enableOutline(TRUE);
//if 0				zoomMarkerKey = pPlot->insertMarker();
//if 0				pPlot->setMarkerPen(zoomMarkerKey, QPen(red));
//if 0				pPlot->setMarkerLineStyle(zoomMarkerKey, QwtMarker::HLine);
//if 0		        pPlot->setOutlineStyle(Qwt::HLine);
				pPlot->replot();
				break;		
			
			case REC_ZOOM:
//if 0				pPlot->enableOutline(TRUE);
//if 0		        pPlot->setOutlineStyle(Qwt::Rect);
				break;
			
			case ELL_ZOOM:
//if 0				pPlot->enableOutline(TRUE);
//if 0		        pPlot->setOutlineStyle(Qwt::Ellipse);
				break;
		}
	}
}

void WorkshopPlot2D::plotMouseReleased( const QMouseEvent& e)
{
	if(e.button()==Qt::RightButton)
	{
		if(highlightList.isEmpty())
			return;
		if(!pNewMeasure)
			pNewMeasure = new WorkshopMeasure(QString::null, VIRTUAL_VALUE, 1000);
		
		WorkshopMeasure *pwm;
		Q3ValueList<long>::iterator it;
		for ( it = highlightList.begin(); it != highlightList.end(); ++it )
		{
			// getting highlighted measure
			pwm = curveDict[*it];
			// resizing if needed
			if(pNewMeasure->size()!=pwm->size())
				pNewMeasure->resize(pwm->size());
			// updating values of pNewMeasure
			pNewMeasure->at(0) = pwm->at(0);
			unsigned int i=0;
			for(i=1; i<pNewMeasure->size(); i++)
				pNewMeasure->at(i) = (pwm->at(i-1)+pwm->at(i))/2;
			QString label(pwm->getLabel());
			label += "->moyenne";
			pNewMeasure->setLabel(label);
			pNewMeasure->setShortLabel("sample");
			pNewMeasure->actuateDate();
			pNewMeasure->validate();
			
			enableDisplayNewMeasure();
			
			emit newMeasure(pNewMeasure);
		}
 

		printf("click droit\n");
		return;
	}
	if(e.button()==Qt::MidButton)
	{
		printf("click milieu\n");
		return;
	}
	
	if(e.button()==Qt::LeftButton)
	{
		switch(zoomMode)
		{
			////////////
		case SELECT:
			 onDoHighlight(e);		
			/////////////
			break;
		case NO_ZOOM:
	//		onDoHighlight(e);
        // update cursor pos display
            plotMouseMoved(e);
            //pPlot->enableOutline(FALSE);			
			break;
		case ELL_ZOOM:
		case REC_ZOOM:
			onZoomRec(e);
			break; 
		case H_ZOOM:
//if 0			pPlot->removeMarker(zoomMarkerKey);
			onHZoom(e);
			break;
		case V_ZOOM:
//if 0			pPlot->removeMarker(zoomMarkerKey);
			onVZoom(e);
			break;
			// select a curve
			//printf("x=%g, y=%g \n", pPlot->invTransform(QwtPlot::xBottom, e.pos().x()), pPlot->invTransform(QwtPlot::yLeft, e.pos().y()));
		}
		pPlot->replot();
	}
}

void WorkshopPlot2D::slotZoom(QAction* onAction)
{
	if(onAction==Select)
		zoomMode = SELECT;		
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
	else if(onAction==CursorForm)
		{
		    zoomMode = CURSOR_FOLLOW;
		}
	else
		zoomMode = NO_ZOOM;
}

void WorkshopPlot2D::slotAutoX(bool on)
{
	autoscaleX = on;
	if(on)
		pPlot->setAxisAutoScale(QwtPlot::xBottom);
	
	pPlot->replot();
}

void WorkshopPlot2D::slotAutoY(bool on)
{
	autoscaleY = on;
	
	if(on)
		pPlot->setAxisAutoScale(QwtPlot::yLeft);	

	pPlot->replot();	
}

void WorkshopPlot2D::onDoHighlight(const QMouseEvent& e)
{
	int d;
	long key = 0; //if 0 pPlot->closestCurve( e.pos().x(), e.pos().y(), d);
	if((!key)||(d>MAX_SELECT_DIST))
	{
		// remove all highlighted curves
		clearHighlight();
		pPlot->replot();
		return;
	}
	else
	{
		if(isHighlighted(key))
			removeHighlight(key);
		else
			addHighlight(key);

		pPlot->replot();
	}
}

void WorkshopPlot2D::onZoomRec(const QMouseEvent& e)
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

void WorkshopPlot2D::onHZoom(const QMouseEvent& e)
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

void WorkshopPlot2D::onVZoom(const QMouseEvent& e)
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

/* slot for filter window creation
*/
void WorkshopPlot2D::slotFilters()
{
	printf("slotFilters\n");

	loadFilterManager();
}

int WorkshopPlot2D::loadFilterManager()
{
	if(!filterManager)
	{//modif!!
		filterManager = new SwFilterManager(pWorkspace, "filterManager", 0);
		connect(filterManager, SIGNAL(selectedFilterChanged()), this, SLOT(slotUpdateImage()));
	}
	else
	{
		printf("filtermanager already loaded !!\n");
		filterManager->showWindow();
	}

	return 1;
}
