#include "workshopplot2d.h"

// pixmaps includes
#include "iconSelection.xpm"
#include "IconMoveView.xpm"
#include "iconZoomRec.xpm"
#include "iconZoomVert.xpm"
#include "iconZoomHor.xpm"
#include "iconZoomEll.xpm"
#include "iconAutoX.xpm"
#include "iconAutoY.xpm"
#include "IconSelCurve.xpm"
#include "IconCurveMarker.xpm"
#include "IconCurveProp.xpm"

#include "IconFilters.xpm"

WorkshopPlot2D::WorkshopPlot2D(QWidget* parent, const char *name, int wflags): 
        WorkshopTool(parent, name, wflags)
{
	
	
	view_back = new QHBox(pWin);
	view_back->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
	
	initTool();
	initPlot();
	initVars();
	initActions();
	initToolBar();
	
	connect(this, SIGNAL(newMeasure(int)), SLOT(onNewMeasure(int)));
	connect(this, SIGNAL(rmMeasure(int)), SLOT(onDelMeasure(int)));
	
	filterManager = NULL;
	pWorkspace=parent;
	
	pWin->show();
}

WorkshopPlot2D::WorkshopPlot2D(WorkshopMeasure* pDoc, QWidget* parent, const char *name, int wflags): 
        WorkshopTool(parent, name, wflags)
{
	pWin->setCaption(name);
	//pWorkspace=parent;

	view_back = new QHBox(pWin);
	view_back->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
	
	initTool();
	initPlot();
	initVars();
	initActions();
	initToolBar();
	filterManager = NULL;

	connect(this, SIGNAL(newMeasure(int)), SLOT(onNewMeasure(int)));
	connect(this, SIGNAL(rmMeasure(int)), SLOT(onDelMeasure(int)));

	addNewMeasure(pDoc);
	pWin->setMinimumSize(600, 200);
	pWin->updateGeometry();
 
	pWin->show();
}

WorkshopPlot2D::~WorkshopPlot2D()
{
	if(markerForm)
	   delete markerForm;
	if(pPlot)
		 delete pPlot;
	if(my_cursor)
		 delete my_cursor;
	if(zoomToolBar)
		 delete zoomToolBar;
	if(properties)
		delete properties;
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
	pPlot->setMinimumSize(600, 200);
	pPlot->updateGeometry();
	
	// set plot mode
	pPlot->set_plotMode(GRAPH_MODE);
	// outline
	pPlot->enableOutline(TRUE);
    pPlot->setOutlinePen(red);
	
	////////////////////////////////////////	
	// Add a cursor
 	//my_cursor = new QwtExtCursor(pPlot );
    //my_cursor->setPen( QPen( red, 1, Qt::SolidLine ) );
	///////////////////////////////////////////////////

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
 
	// setting true for auto delete of HPenDict
	HPenDict.setAutoDelete(true);
	
	//autoscaleX = true;
	//autoscaleY = true;
	autoscaleX = false;
	autoscaleY = false;
	zoomMode = NO_ZOOM;	
	markerForm=NULL;
	
	my_cursor=NULL;
	
	properties=NULL;
	
	pressed=FALSE;
}

void WorkshopPlot2D::initActions()
{
	QPixmap iconSelect, iconMove;
	QPixmap iconZoomV, iconZoomH, iconZoomR, iconZoomE;
	QPixmap iconAutoScaleX, iconAutoScaleY;
	QPixmap iconMarkerForm, iconPropForm;
	
	iconSelect = QPixmap(IconSelCurve_xpm);
	iconMove = QPixmap(IconMoveView_xpm);
	iconZoomR = QPixmap(iconZoomRec);
	iconZoomV = QPixmap(iconZoomVert);
	iconZoomH = QPixmap(iconZoomHor);
	iconZoomE = QPixmap(iconZoomEll);
	iconAutoScaleX = QPixmap(iconAutoX);
	iconAutoScaleY = QPixmap(iconAutoY);
	iconMarkerForm = QPixmap(IconCurveMarker_xpm);
	iconPropForm = QPixmap(IconCurveProp_xpm);

	// creating group of zoom actions
	grpZoom = new QActionGroup( this );
	grpZoom->setExclusive( TRUE );
	connect( grpZoom, SIGNAL(selected(QAction*)), this, SLOT(slotZoom(QAction*)) );
	
	Select = new QAction(tr("Selection tool"), iconSelect, tr("&Selection"), 0, grpZoom);
	Select->setStatusTip(tr("Select a curve"));
	Select->setWhatsThis(tr("You can select on ore many curves !"));
	Select->setToggleAction (true);

	Move = new QAction(tr("Moving plot"), iconMove, tr("&Move"), 0, grpZoom);
	Move->setStatusTip(tr("Dragging the curves on plot"));
	Move->setWhatsThis(tr("You can grag the curves !"));
	Move->setToggleAction (true);

	ZoomR = new QAction(tr("Rectangular zoom"), iconZoomR, tr("&Rectangular"), 0, grpZoom);
	ZoomR->setStatusTip(tr("Zoom on a rectangular area"));
	ZoomR->setWhatsThis(tr("Designate a rectangular area and zoom !"));
	ZoomR->setToggleAction (true);

	ZoomV = new QAction(tr("Vertical zoom"), iconZoomV, tr("&Vertical"), 0, grpZoom);
	ZoomV->setStatusTip(tr("Zoom on a vertical area"));
	ZoomV->setWhatsThis(tr("Designate a vertical area and zoom !"));
	ZoomV->setToggleAction (true);
	
	ZoomH = new QAction(tr("Horizontal zoom"), iconZoomH, tr("&Horizontal"), 0, grpZoom);
	ZoomH->setStatusTip(tr("Zoom on a horizontal area"));
	ZoomH->setWhatsThis(tr("Designate a horizontal area and zoom !"));
	ZoomH->setToggleAction (true);
	
	ZoomE = new QAction(tr("Elliptic zoom"), iconZoomE, tr("&Elliptic"), 0, grpZoom);
	ZoomE->setStatusTip(tr("Zoom on an elliptic area"));
	ZoomE->setWhatsThis(tr("Designate an Elliptic area and zoom !"));
	ZoomE->setToggleAction (true);

	AutoScaleX = new QAction(tr("Autoscale X"), iconAutoScaleX, tr("&Autoscale X"), 0, this);
	AutoScaleX->setStatusTip(tr("Autoscale on X axe"));
	AutoScaleX->setWhatsThis(tr("Autoscale on X axe"));
	AutoScaleX->setToggleAction (true);
	AutoScaleX->setOn (false);
	connect(AutoScaleX, SIGNAL(toggled(bool)), this, SLOT(slotAutoX(bool)));

	AutoScaleY = new QAction(tr("Autoscale Y"), iconAutoScaleY, tr("&Autoscale Y"), 0, this);
	AutoScaleY->setStatusTip(tr("Autoscale on Y axe"));
	AutoScaleY->setWhatsThis(tr("Autoscale on Y axe"));
	AutoScaleY->setToggleAction (true);
	AutoScaleY->setOn (false);
	connect(AutoScaleY, SIGNAL(toggled(bool)), this, SLOT(slotAutoY(bool)));
	
	/////////////////////////////////////
	MarkForm = new QAction(tr("Marker Form"),iconMarkerForm , tr("&Marker Form"), 0, this);
	MarkForm->setStatusTip(tr("Set the Marker Form"));
	MarkForm->setWhatsThis(tr("Set the Marker Form"));
	//MarkForm->setToggleAction (true);
	MarkForm->setToggleAction (false);
	MarkForm->setOn (false);
	connect(MarkForm, SIGNAL(activated()), this, SLOT(slotMarkerForm()));
	
	PropForm = new QAction(tr("Properties Form"), iconPropForm, tr("&Properties Form"), 0, this);
	PropForm->setStatusTip(tr("Set the Properties Form"));
	PropForm->setWhatsThis(tr("Set the Properties Form"));
	PropForm->setToggleAction (false);
	PropForm->setOn (false);
	connect(PropForm, SIGNAL(activated()), this, SLOT(slotPropertiesForm()));


    // FILTERS
    QPixmap iconFilters = QPixmap(IconFilters_xpm);
	filtersAction = new QAction(tr("Filters Window"), iconFilters, tr("&Filters Window"), 0, this);
	filtersAction->setStatusTip(tr("Launch filters window"));
	filtersAction->setWhatsThis(tr("Launch filters window"));
	filtersAction->setToggleAction (false);
	filtersAction->setOn (false);
	connect(filtersAction, SIGNAL(activated()), this, SLOT(slotFilters()));

	////////////////////////////////////
}

///////////////////////////////////
void WorkshopPlot2D::slotMarkerForm()
{
	printf("toto!\n");
    zoomMode = MARKER_FORM;		
	
	if(!markerForm)
	    markerForm = new MarkerDialogForm( pPlot, "marker dialog" );
	else
		printf("markerdialog already loaded!\n");
	
	markerForm->init(pPlot);
	connect( my_cursor, SIGNAL( cursor_pos( double, double ) ), markerForm,
                                      SLOT( set_MarkerPos(double, double) ) );
	 markerForm->show();
}


////////////////////////////
void WorkshopPlot2D::slotPropertiesForm()
{
	
	printf("SLOTPROPERTIESFORMS!\n");
    if(!highlightList.isEmpty())
	 {
		
		if(!properties)
		{
	    	properties = new PropertiesForm( pPlot, "properties" );
	    	printf("liste surbrillante non vide!!\n");
		}
			
		QValueList<long>::iterator it;
		for ( it = highlightList.begin(); it != highlightList.end(); ++it )
		{
			
	/*
		    QString name;
    		QwtSymbol symbol;
	    
    		name = pPlot->curveTitle( *it );
    		symbol = pPlot->curveSymbol( *it );
    		QPen curvePen;
    		curvePen = pPlot->curvePen( *it );
    		int curveStyle;
    		curveStyle = pPlot->curveStyle( *it );
      	 
    		properties->init(&name, &curvePen, &curveStyle, &symbol);
    		properties->exec();	

    		pPlot->setCurveSymbol(*it, symbol);
    		pPlot->setCurveTitle(*it, name);
    		pPlot->setCurvePen(*it, curvePen);
    		pPlot->setCurveStyle(*it, curveStyle);
    		pPlot->replot();
	*/
		}
	}
	else
		printf("liste surbrillante vide!!\n");
}

///////////////////////////////////



void WorkshopPlot2D::initToolBar()
{
	// creating toolbar
	zoomToolBar = new QToolBar(pWin, "zoom operations");
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
}

void WorkshopPlot2D::clearHighlight()
{
	QIntDictIterator<QPen> it(HPenDict);
    for ( ; it.current(); ++it )
	{
		pPlot->setCurvePen(it.currentKey(), *it.current());
	}
	
	highlightList.clear();	
	HPenDict.clear();
}

void WorkshopPlot2D::removeHighlight(long key)
{
	pPlot->setCurvePen(key, *HPenDict[key]);
	
	highlightList.remove(key);	
	HPenDict.remove(key);
}

void WorkshopPlot2D::addHighlight(long key)
{
	highlightList.append(key);
	QPen *pen = new QPen(pPlot->curvePen(key));
	HPenDict.insert(key, pen);
	QPen newpen (pPlot->curvePen(key));
	newpen.setColor(Qt::yellow);
	pPlot->setCurvePen(key, newpen);
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
	QIntDictIterator<WorkshopMeasure> it(curveDict);
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
	double dx, dy, xmin, xmax, ymin, ymax, xval, yval;
	
	switch(zoomMode)
	{
		default:
			break;
		case MOV_ZOOM:
	
			dx = pPlot->invTransform(QwtPlot::xBottom, e.pos().x()) - pPlot->invTransform(QwtPlot::xBottom, memPoint.x());
			dy = pPlot->invTransform(QwtPlot::yLeft, e.pos().y()) - pPlot->invTransform(QwtPlot::yLeft, memPoint.y());
			xmin = pPlot->axisScale(QwtPlot::xBottom)->lBound() - dx;
			xmax = pPlot->axisScale(QwtPlot::xBottom)->hBound() - dx;
			ymin = pPlot->axisScale(QwtPlot::yLeft)->lBound() - dy;
			ymax = pPlot->axisScale(QwtPlot::yLeft)->hBound() - dy;
			pPlot->setAxisScale(QwtPlot::xBottom, xmin, xmax);
			pPlot->setAxisScale(QwtPlot::yLeft, ymin, ymax);
			memPoint = e.pos();
			pPlot->replot();
		    break;
		/*
		case SELECT:
			// Recupere les valeurs du point le plus proche et trace une croix (marqueur)
    	    if(pressed)
    			{
      				my_cursor->print( crv, e.pos().x(), e.pos().y(), xval, yval );
    			}
			break;
	*/
	}
}


void WorkshopPlot2D::plotMousePressed( const QMouseEvent& e)
{
	int dist;
	
	if(e.button()==LeftButton)
	{
		// store position
		memPoint = e.pos();
 
		switch(zoomMode)
		{
			default:
				break;
				/////////////////////////////////
			case SELECT:
                  //crv = pPlot->closestCurve( e.pos().x(), e.pos().y(), dist );
                  // On met le flag pressed à TRUE
             //     if( e.button() == LeftButton )
             //           pressed = TRUE;
                  // update cursor pos display
                  //plotMouseMoved(e);
                 pPlot->enableOutline(FALSE);		
			
				 break;			
			    //////////////////////////////////////
			case MOV_ZOOM:
				pPlot->enableOutline(TRUE);	
			    pPlot->setOutlineStyle(Qwt::Cross);
			
			case NO_ZOOM:
				pPlot->enableOutline(TRUE);	
		        pPlot->setOutlineStyle(Qwt::Cross);
				break;
			case V_ZOOM:
				pPlot->enableOutline(TRUE);	
				zoomMarkerKey = pPlot->insertMarker();
				pPlot->setMarkerPen(zoomMarkerKey, QPen(red));
				pPlot->setMarkerLineStyle(zoomMarkerKey, QwtMarker::VLine);
		        pPlot->setOutlineStyle(Qwt::VLine);
				pPlot->replot();
				break;
			case H_ZOOM:
				pPlot->enableOutline(TRUE);	
				zoomMarkerKey = pPlot->insertMarker();
				pPlot->setMarkerPen(zoomMarkerKey, QPen(red));
				pPlot->setMarkerLineStyle(zoomMarkerKey, QwtMarker::HLine);
		        pPlot->setOutlineStyle(Qwt::HLine);
				pPlot->replot();
				break;			
			case REC_ZOOM:
				pPlot->enableOutline(TRUE);	
		        pPlot->setOutlineStyle(Qwt::Rect);
				break;
			case ELL_ZOOM:
				pPlot->enableOutline(TRUE);	
		        pPlot->setOutlineStyle(Qwt::Ellipse);
				break;
		}
	}
}

void WorkshopPlot2D::plotMouseReleased( const QMouseEvent& e)
{
	if(e.button()==RightButton)
	{
		if(highlightList.isEmpty())
			return;
/*
		if(!pNewMeasure)
			pNewMeasure = new WorkshopMeasure(QString::null, VIRTUAL_VALUE, 1000);
		
		WorkshopMeasure *pwm;
		QValueList<long>::iterator it;
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
 
*/

		printf("click droit\n");
		return;
	}

	
	if(e.button()==MidButton)
	{
		printf("click milieu\n");
		return;
	}
	
	if(e.button()==LeftButton)
	{
		switch(zoomMode)
		{
			////////////
		case SELECT:
             //pressed = FALSE;	
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
			pPlot->removeMarker(zoomMarkerKey);
			onHZoom(e);
			break;
		case V_ZOOM:
			pPlot->removeMarker(zoomMarkerKey);
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
	long key = pPlot->closestCurve( e.pos().x(), e.pos().y(), d);
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
		filterManager = new SwFilterManager(pWorkspace, "filterManager",0);
		connect(filterManager, SIGNAL(selectedFilterChanged()), this, SLOT(slotUpdateImage()));
	}
	else
	{
		printf("filtermanager already loaded !!\n");
		filterManager->showWindow();
	}

	return 1;
}
