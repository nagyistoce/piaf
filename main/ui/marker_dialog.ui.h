/**************************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename slots use Qt Designer which will
** update this file, preserving your code. Create an init() slot in place of
** a constructor, and a destroy() slot in place of a destructor.
 *************************************************************************************/

void MarkerDialogForm::init(QwtExtPlot *plot)
{
  ListBox->setSelectionMode( Q3ListBox::Single );
  // set default properties
  default_symbol.setStyle( QwtSymbol::None );
  xpos = 0;
  ypos = 0;

  my_plot=plot;
  if( !my_plot )
  {
    printf( "/nErreur pointeur my_plot" );
    return;
  }

  //symbol properties
  my_symbol.setSize( 6, 6 );
  symbol_pen.setWidth( 1 );

  //implement the listbox
  QListBoxMarker *pQlbm;
  mrk_keys = my_plot->markerKeys();
  int mode, style, symbol;
  
  if( mrk_keys.size() != 0 )
  {
    for( uint i = 0; i<mrk_keys.size(); i++ )
    {
      if( !ListBox->item( i ) )
      {
        purchaseStyle( mrk_keys[i], mode, style, symbol );
        pQlbm = new QListBoxMarker(ListBox, my_plot->markerLabel(mrk_keys[i]),
                                   mrk_keys[i], mode, style, symbol);
        //	ListBox->setCurrentItem( pQlbm );
      }
    }
  }
  else
    pCurrLbm = NULL;

  //connect(plot, SIGNAL(markerPosSet(int,int)), this, SLOT(setPos()));
}

void MarkerDialogForm::destroy()
{
}

/**********************************************************************************
* Create a new marker and the associated QListBoxItem.                             *
 *********************************************************************************/
void MarkerDialogForm::newMarker()
{
 // set label
  QString aLabel = "New Marker";
  //xposEdit->setText( QString( "%1" ).arg( xpos, 0, 'f', 3 ) );
  //yposEdit->setText( QString( "%1" ).arg( ypos, 0, 'f', 3 ) );
  xpos = xposEdit->text().toDouble();
  ypos = yposEdit->text().toDouble();
  // create new marker
  long l_key = my_plot->insertMarker( aLabel );
  if(!l_key)
  {
    return;
  }
  my_plot->setMarkerPos( l_key, xpos, ypos );
  my_plot->replot();

  // creating a QListBoxMarker
  QListBoxMarker * pQlbm = new QListBoxMarker( ListBox, aLabel, l_key, 0, 0, 0 );
  ListBox->setCurrentItem(pQlbm);
}

/**********************************************************************************
* Remove the selected marker and its associated QListBoxItem                       *
**********************************************************************************/
void MarkerDialogForm::removeMarker()
{
  // getting current item index...
  int indice = ListBox->currentItem();

  // getting associated marker
  long l_key = pCurrLbm->key();
  if(l_key<0)
    return;

  // remove marker
  if(!my_plot->removeMarker(l_key))
  {
    printf("error : unable to remove marker with key : %ld\n", l_key);
    return;
  }

  // remove QListBoxMarker
  ListBox->removeItem(indice);
  my_plot->replot();
}

/**********************************************************************************
* find the current highlighted item and display its properties                     *
 *********************************************************************************/
void MarkerDialogForm::itemSelected( Q3ListBoxItem *pQlbi )
{
  // setting new current item
  pCurrLbm = (QListBoxMarker *)pQlbi;
  long l_key = pCurrLbm->key();

  //print the current properties values
  QColor mrk_color = my_plot->markerLinePen( l_key ).color();
  QColor symb_color = my_plot->markerSymbol( l_key ).pen().color();

  mrk_pen.setColor( mrk_color );
  symbol_pen.setColor( symb_color );
  
  //show the color selected by changing button's background color
  colorPushButton->setPaletteBackgroundColor( mrk_color );
  PushButton6->setPaletteBackgroundColor( symb_color );

  //find and show the label and the position of the current marker
  double x, y;
  my_plot->markerPos( l_key, x, y );
  xposEdit->setText( QString( "%1" ).arg( x, 0, 'f', 3 ) );
  yposEdit->setText( QString( "%1" ).arg( y, 0, 'f', 3 ) );
  NameEdit->setText( pCurrLbm->text() );

  //show the plot properties of the current marker
  modeComboBox->setCurrentItem( pCurrLbm->mode() );
  styleComboBox->setCurrentItem( pCurrLbm->markerStyle() );
  symbolComboBox->setCurrentItem( pCurrLbm->symbolStyle() );
  mrk_pen.setWidth( my_plot->markerLinePen( l_key ).width() );
  widthSpinBox->setValue( mrk_pen.width() );
}

void MarkerDialogForm::setMarkerLabel()
{
  if( pCurrLbm && (ListBox->numItemsVisible() != 0) )
  {
     // getting new label
    QString aLabel = NameEdit->text();    
    // getting current label
    QString cLabel = pCurrLbm->text();

    // setting new label
    if( aLabel != cLabel )
    {
      pCurrLbm->setText(aLabel);
      ListBox->updateContents();
    }
  }
}

void MarkerDialogForm::set_MarkerPos( double x, double y )
{
    //store the values
    xpos = x;
    ypos = y;

    //show the values in the appropriated lines edit
    xposEdit->setText( QString( "%1" ).arg( x, 0, 'f', 3 ) );
    yposEdit->setText( QString( "%1" ).arg( y, 0, 'f', 3 ) );
}

void MarkerDialogForm::setMarkerXPos()
{
   xpos = xposEdit->text().toDouble();
}

void MarkerDialogForm::setMarkerYPos()
{
   ypos = yposEdit->text().toDouble();
}

void MarkerDialogForm::setMarkerWidth()
{
    mrk_pen.setWidth( widthSpinBox->value() );
}

void MarkerDialogForm::setMarkerColor()
{
    QColor mrk_color = select->getColor();
    mrk_pen.setColor( mrk_color );
    colorPushButton->setPaletteBackgroundColor( mrk_color );
}

void MarkerDialogForm::setMarkerMode( int value )
{
    if( pCurrLbm )
    {
       switch( value )
       {
         case 0:  // No Line
           line_style = QwtMarker::NoLine;
           break;
         case 1:  // Horizontal
           line_style = QwtMarker::HLine;
           break;
         case 2:  // Vertical
           line_style = QwtMarker::VLine;
           break;
         case 3:  // Cross
           line_style = QwtMarker::Cross;
           break;
         default:
           line_style = QwtMarker::NoLine;
           break;
       }
       pCurrLbm->setMode( value );
   } 
}

void MarkerDialogForm::setMarkerStyle( int value )
{
    if( pCurrLbm )
    {
       switch( value )
       {
         case 0:  // SolidLine
           mrk_pen.setStyle( Qt::SolidLine );
           break;
         case 1:  //DashLine
           mrk_pen.setStyle( Qt::DotLine );
           break;
         case 2:  //DotLine
           mrk_pen.setStyle( Qt::DashLine );
           break;
         case 3:  //DashDotLine
           mrk_pen.setStyle( Qt::DashDotLine );
           break;
         case 4:  //DashDotDotLine
           mrk_pen.setStyle( Qt::DashDotDotLine );
           break;
         default:
           mrk_pen.setStyle( Qt::SolidLine );
           break;
       }
       pCurrLbm->setMarkerStyle( value );
   }
}

void MarkerDialogForm::setSymbolColor()
{
    QColor symb_color = select->getColor();
    symbol_pen.setColor( symb_color );
    PushButton6->setPaletteBackgroundColor( symb_color );
}

void MarkerDialogForm::setMarkerSymbol( int value )
{
  if( pCurrLbm )
  {
    switch(value)
    {
     case 0:  // None
         my_symbol.setStyle( QwtSymbol::None );
         break;
     case 1:  //Ellipse
         my_symbol.setStyle( QwtSymbol::Ellipse );
         break;
     case 2:  //Cross
         my_symbol.setStyle( QwtSymbol::XCross );
         break;
     case 3:  //Diamond
         my_symbol.setStyle( QwtSymbol::Diamond );
         break;
     case 4:  //Rectangle
         my_symbol.setStyle( QwtSymbol::Rect );
         break;
     case 5:  //Triangle
         my_symbol.setStyle( QwtSymbol::Triangle );
         break;
     default:
         my_symbol.setStyle( QwtSymbol::None );
         break;
    } 
    pCurrLbm->setSymbolStyle( value );
  }
}

void MarkerDialogForm::applyProperties()
{

    my_symbol.setPen( symbol_pen );

    if ( pCurrLbm )
    {
      long l_key = pCurrLbm->key();
      QString l_label = pCurrLbm->text();

      // set marker properties
      my_plot->setMarkerLineStyle( l_key, line_style );
      my_plot->setMarkerLinePen( l_key, mrk_pen );
      my_plot->setMarkerPen( l_key, mrk_pen );

      my_plot->setMarkerLabelPen( l_key, mrk_pen );
      my_plot->setMarkerLabel( l_key,l_label );
      my_plot->setMarkerLabelAlign( l_key, Qt::AlignRight|Qt::AlignBottom );
      
      my_plot->setMarkerSymbol( l_key, my_symbol );

      my_plot->setMarkerPos( l_key, xposEdit->text().toDouble(), yposEdit->text().toDouble() );
      my_plot->replot();
    }
}

void MarkerDialogForm::closeMarkerDialog()
{
    my_plot->replot();
    close();
}

void MarkerDialogForm::purchaseStyle( long key, int & mode, int & style, int & symbol )
{
  switch( my_plot->markerLineStyle( key ) )
  {
    case QwtMarker::NoLine:
         mode = 0;
         break;
     case QwtMarker::HLine:
         mode = 1;
         break;
     case QwtMarker::VLine:
         mode = 2;
         break;
     case QwtMarker::Cross:
         mode = 3;
         break;
     default:
         mode = 0;
         break;
  }
  switch( my_plot->markerLinePen( key ).style() )
  {
     case Qt::SolidLine:
         style = 0;
         break;
     case Qt::DotLine:
         style = 1;
         break;
     case Qt::DashLine:
         style = 2;
         break;
     case Qt::DashDotLine:
         style = 3;
         break;
     case Qt::DashDotDotLine:
         style = 4;
         break;
     default:
         style = 0;
         break;    
  }
  switch( my_plot->markerSymbol( key ).style() )
  {
     case QwtSymbol::None:
         symbol = 0;
         break;
     case QwtSymbol::Ellipse:
         symbol = 1;
         break;
     case QwtSymbol::XCross:
         symbol = 2;
         break;
     case QwtSymbol::Diamond:
         symbol = 3;
         break;
     case QwtSymbol::Rect:
         symbol = 4;
         break;
     case QwtSymbol::Triangle:
         symbol = 5;
         break;
     default:
         symbol = 0;
         break;
  }
}

