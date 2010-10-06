/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename slots use Qt Designer which will
** update this file, preserving your code. Create an init() slot in place of
** a constructor, and a destroy() slot in place of a destructor.
*****************************************************************************/
#include <math.h>

void PropertiesForm::init( const QString *last_name, QPen *last_pen, int *last_style,
                            QwtSymbol *last_symbol )
{
    // Sauvegarde des pointeurs
    pen = last_pen;
    style = last_style;
    name = ( QString * )last_name;
    crv_symbol = last_symbol;

    // il faudra creer un message box qui fermera l'application
    if( !pen )
      printf("Erreur d'allocation du pointeur sur la structure QPen *pen");
    if( !style )
      printf("Erreur d'allocation du pointeur sur la structure int *style");
    if( !name )
      printf( "Erreur d'allocation du pointeur sur la structure QString *name" );
    if( !crv_symbol )
      printf( "Erreur d'allocation du pointeur sur la structure QwtSymbol *crv_symbol" );

    // Affecte les param?res de la courbe s?ectionn? ?la courbe de pr?isualisation
    myPen = *pen;
    symbol_pen = *pen;
    myStyle = *style;
    new_name = *name;
    my_symbol = *crv_symbol;

    NameEdit->setText( new_name );
    int value = ( int )pen->width();
    widthSpinBox->setValue( value );

    int pen_style, crv_style, symbol;

    // Display the pen style into appropriated combo box
    switch( myPen.style() )
    {
     case Qt::SolidLine:
         pen_style = 0;
         break;
     case Qt::DashLine:
         pen_style = 1;
         break;
     case Qt::DotLine:
         pen_style = 2;
         break;
     case Qt::DashDotLine:
         pen_style = 3;
         break;
     case Qt::DashDotDotLine:
         pen_style = 4;
         break;
     default:
         pen_style = 0;
         break;
    }
    penComboBox->setCurrentItem( pen_style );

    // Display the curve style into appropriated combo box
    switch( myStyle )
    {
     case QwtCurve::Lines:
         crv_style = 0;
         break;
     case QwtCurve::Sticks:
         crv_style = 1;
         break;
     case QwtCurve::Steps:
         crv_style = 2;
         break;
     case QwtCurve::Dots:
         crv_style = 3;
         break;
     case QwtCurve::Spline:
         crv_style = 4;
         break;
     default:
         crv_style = 0;
         break;
    }
    curveComboBox->setCurrentItem( crv_style );

    // Display the symbol style into appropriated combo box
    switch( my_symbol.style() )
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
    plotComboBox->setCurrentItem( symbol );

    // Initialisation of the plot area
    myPlot = new QwtExtPlot( this, "local view plot" );
    myPlot->setGeometry( QRect( -40, 170, 445, 195 ) );
    myPlot->setTitle( "Signal Preview" );
    myPlot->setAutoLegend( TRUE );
    myPlot->setOutlinePen( Qt::NoPen );

    myPlot->set_plotMode( XY_MODE );
    myPlot->show();
    curveID = myPlot->insertCurve( "Example" );

    // Same settings than the curve we want to modify
    myPlot->setCurveSymbol( curveID, my_symbol );
    myPlot->setCurvePen( curveID, *pen );
    myPlot->setCurveStyle( curveID, *style );

    // f(x) = 2*sin(x)
    for( int i=0; i<50; i++ )
    {
            y[i] = 2*sin( i*0.5 );
            x[i] = i*0.5;
    }
    myPlot->setCurveRawData( curveID, x, y, 50 );
    myPlot->replot();
}

void PropertiesForm::destroy()
{
    delete myPlot;
}

void PropertiesForm::setCurveName()
{
    /*QString*/ new_name = NameEdit->text();
    /*if( new_name != name )
        new_name = name;*/
}

void PropertiesForm::setColor()
{
    myPen.setColor( select->getColor() );
    myPlot->setCurvePen( curveID, myPen );
    myPlot->replot();
}

void PropertiesForm::setDotsColor()
{
    symbol_pen.setColor( select->getColor() );
    my_symbol.setPen( symbol_pen );
    myPlot->setCurveSymbol( curveID, my_symbol );
    myPlot->replot();
}

void PropertiesForm::setWidth()
{
    int l_width = widthSpinBox->value();
    if( (int)myPen.width() != l_width )
    {
      myPen.setWidth( l_width );
      myPlot->setCurvePen( curveID, myPen );
      myPlot->replot();
    }
}

void PropertiesForm::set_PenStyle( int value )
{
    switch( value )
    {
     case 0:  // SolidLine
         myPen.setStyle( Qt::SolidLine );
         break;
     case 1:  //DashLine
         myPen.setStyle( Qt::DashLine );
         break;
     case 2:  //DotLine
         myPen.setStyle( Qt::DotLine );
         break;
     case 3:  //DashDotLine
         myPen.setStyle( Qt::DashDotLine );
         break;
     case 4:  //DashDotDotLine
         myPen.setStyle( Qt::DashDotDotLine );
         break;
     default:
         myPen.setStyle( Qt::SolidLine );
         break;
    }
    myPlot->setCurvePen( curveID, myPen );
    myPlot->replot();
}

void PropertiesForm::set_CurveStyle( int value )
{
    switch( value )
    {
     case 0:  // Lines
         myStyle = QwtCurve::Lines;
         break;
     case 1:  //Sticks
         myStyle = QwtCurve::Sticks;
         break;
     case 2:  //Steps
         myStyle = QwtCurve::Steps;
         break;
     case 3:  //Dots
         myStyle = QwtCurve::Dots;
         break;
     case 4:  //Splines
         myStyle = QwtCurve::Spline;
         break;
     default:
         myStyle = QwtCurve::Lines;
         break;
    }
    myPlot->setCurveStyle( curveID, myStyle );
    myPlot->replot();
}

void PropertiesForm::drawMarker( int value )
{
  switch( value )
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
  symbol_pen.setWidth( 2 );
  symbol_pen.setStyle( Qt::SolidLine );
  my_symbol.setSize( 7,7 );
  my_symbol.setPen( symbol_pen );
  myPlot->setCurveSymbol( curveID, my_symbol );
  myPlot->replot();
}

void PropertiesForm::validProperties()
{
    *name = new_name;
    *pen = myPen;
    *style = myStyle;
    *crv_symbol = my_symbol;
    close();
}
