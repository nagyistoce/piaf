/****************************************************************************
** Form implementation generated from reading ui file 'curve_properties.ui'
**
** Created: jeu jan 23 09:37:55 2003
**      by:  The User Interface Compiler (uic)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/
#include "curve_properties.h"

#include <qvariant.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qimage.h>
#include <qpixmap.h>

#include "curve_properties.ui.h"
static const char* const image0_data[] = {
"35 17 2 1",
". c None",
"# c #000000",
"...................................",
"......######.......................",
".....#......#......................",
"....#........#.....................",
"...#..........##...................",
"..#.............#..................",
".#...............#.................",
"#.................#................",
"...................#...............",
"....................#.............#",
".....................#...........#.",
"......................#.........#..",
".......................##......#...",
".........................###..#....",
"............................##.....",
"...................................",
"..................................."};

static const char* const image1_data[] = {
"35 17 2 1",
". c None",
"# c #000000",
"...................................",
".............................#.....",
".............................#.....",
".......................#.....#.....",
".......................#.....#.....",
".......................#.....#....#",
".......................#.....#....#",
"#......................#.....#....#",
"#.....#.....#....#.....#.....#....#",
"......#.....#....#.................",
"......#.....#....#.................",
"......#.....#......................",
"......#.....#......................",
"......#.....#......................",
"............#......................",
"............#......................",
"..................................."};

static const char* const image2_data[] = {
"35 17 2 1",
". c None",
"# c #000000",
"...................................",
"...............................####",
"###............................#...",
"..#......................#######...",
"..#......................#.........",
"..#......................#.........",
"..#......................#.........",
"..#######................#.........",
"........#................#.........",
"........#................#.........",
"........#..........#######.........",
"........#..........#...............",
"........#..........#...............",
"........#######....#...............",
"..............#....#...............",
"..............######...............",
"..................................."};

static const char* const image3_data[] = {
"35 17 2 1",
". c None",
"# c #000000",
"...................................",
"......#....#.......................",
"...................................",
"...................................",
"...................................",
"...................................",
".................#.................",
"#..................................",
"...................................",
"..................................#",
"...................................",
"...................................",
".......................#...........",
"...................................",
".............................#.....",
"...................................",
"..................................."};

static const char* const image4_data[] = {
"35 17 2 1",
". c None",
"# c #000000",
"...................................",
".......#####.......................",
".....###...###.....................",
"....##.......##....................",
"...##.........##...................",
"..##...........##..................",
".##.............#..................",
"##...............#.................",
"#.................#................",
"..................##...............",
"...................##.............#",
"....................##...........##",
".....................##.........##.",
"......................##.......##..",
".......................###...##....",
".........................#####.....",
"..................................."};

static const char* const image5_data[] = {
"35 10 3 1",
". c None",
"# c #000000",
"a c #a0a0a4",
"...................................",
"...................................",
"..####.....####.....####.....####..",
".##aa##...##aa##...##aa##...##aa##.",
".#aaaa#...#aaaa#...#aaaa#...#aaaa#.",
".#aaaa#...#aaaa#...#aaaa#...#aaaa#.",
".##aa##...##aa##...##aa##...##aa##.",
".#####....#####....#####....#####..",
"...................................",
"..................................."};

static const char* const image6_data[] = {
"35 10 2 1",
". c None",
"# c #000000",
"...................................",
"...................................",
"......#...#....#...#....#...#....#.",
"..#..#.....#..#.....#..#.....#..#..",
"...##.......##.......##.......##...",
"...##.......##.......##.......##...",
"..#..#.....#..#.....#..#.....#..#..",
".#....#...#....#...#....#...#....#.",
"...................................",
"..................................."};

static const char* const image7_data[] = {
"35 10 3 1",
". c None",
"# c #000000",
"a c #a0a0a4",
"...................................",
"...................................",
"...#........#........#........#....",
"..#a#......#a#......#a#......#a#...",
".#aa#.....#aa#.....#aa#.....#aa#...",
"#aaaa#...#aaaa#...#aaaa#...#aaaa#..",
".##a#.....##a#.....##a#.....##a#...",
"...#........#........#........#....",
"...................................",
"..................................."};

static const char* const image8_data[] = {
"35 10 3 1",
". c None",
"# c #000000",
"a c #a0a0a4",
"...................................",
"...................................",
"######...######....######...######.",
"#aaaa#...#aaaa#....#aaaa#...#aaaa#.",
"#aaaa#...#aaaa#....#aaaa#...#aaaa#.",
"#aaaa#...#aaaa#....#aaaa#...#aaaa#.",
"#aaaa#...#aaaa#....#aaaa#...#aaaa#.",
"######...######....######...######.",
"...................................",
"..................................."};

static const char* const image9_data[] = {
"35 10 3 1",
". c None",
"# c #000000",
"a c #a0a0a4",
"...................................",
"...................................",
"...#........#........#........#....",
"..##.......##.......##.......##....",
"..#a#......#a#......#a#......#a#...",
".#aa#.....#aa#.....#aa#.....#aa#...",
".#aaa#....#aaa#....#aaa#....#aaa#..",
"######...######...######...######..",
"...................................",
"..................................."};

static const char* const image10_data[] = {
"35 10 2 1",
". c None",
"# c #000000",
"...................................",
"...................................",
"...................................",
"...................................",
"...................................",
"###################################",
"...................................",
"...................................",
"...................................",
"..................................."};

static const char* const image11_data[] = {
"35 10 2 1",
". c None",
"# c #000000",
"...................................",
"...................................",
"...................................",
"...................................",
"...................................",
"#########...#########...#########..",
"...................................",
"...................................",
"...................................",
"..................................."};

static const char* const image12_data[] = {
"35 10 2 1",
". c None",
"# c #000000",
"...................................",
"...................................",
"...................................",
"...................................",
"...................................",
"###..###..###..###..###..###..###..",
"...................................",
"...................................",
"...................................",
"..................................."};

static const char* const image13_data[] = {
"35 10 2 1",
". c None",
"# c #000000",
"...................................",
"...................................",
"...................................",
"...................................",
"...................................",
"#########...###...#########...###..",
"...................................",
"...................................",
"...................................",
"..................................."};

static const char* const image14_data[] = {
"35 10 2 1",
". c None",
"# c #000000",
"...................................",
"...................................",
"...................................",
"...................................",
"...................................",
"#########..###..###..#########..###",
"...................................",
"...................................",
"...................................",
"..................................."};


/*
 *  Constructs a PropertiesForm which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
PropertiesForm::PropertiesForm( QWidget* parent,  const char* name, bool modal, WFlags fl )
    : QDialog( parent, name, modal, fl )
{
    QPixmap image0( ( const char** ) image0_data );
    QPixmap image1( ( const char** ) image1_data );
    QPixmap image2( ( const char** ) image2_data );
    QPixmap image3( ( const char** ) image3_data );
    QPixmap image4( ( const char** ) image4_data );
    QPixmap image5( ( const char** ) image5_data );
    QPixmap image6( ( const char** ) image6_data );
    QPixmap image7( ( const char** ) image7_data );
    QPixmap image8( ( const char** ) image8_data );
    QPixmap image9( ( const char** ) image9_data );
    QPixmap image10( ( const char** ) image10_data );
    QPixmap image11( ( const char** ) image11_data );
    QPixmap image12( ( const char** ) image12_data );
    QPixmap image13( ( const char** ) image13_data );
    QPixmap image14( ( const char** ) image14_data );
    if ( !name )
        setName( "PropertiesForm" );
    setFixedSize( 410, 305 );
    setCaption( trUtf8( "Properties" ) );
    setAcceptDrops( FALSE );
    setSizeGripEnabled( FALSE );

    plotGroupBox = new QGroupBox( this, "plotGroupBox" );
    plotGroupBox->setGeometry( QRect( 0, 0, 200, 80 ) );
    plotGroupBox->setTitle( trUtf8( "Plot options" ) );

    curve_Style = new QLabel( plotGroupBox, "curve_Style" );
    curve_Style->setGeometry( QRect( 10, 20, 40, 20 ) );
    QFont curve_Style_font(  curve_Style->font() );
    curve_Style_font.setBold( TRUE );
    curve_Style->setFont( curve_Style_font );
    curve_Style->setText( trUtf8( "Style" ) );

    Name = new QLabel( plotGroupBox, "Name" );
    Name->setGeometry( QRect( 10, 50, 40, 20 ) );
    QFont Name_font(  Name->font() );
    Name_font.setBold( TRUE );
    Name->setFont( Name_font );
    Name->setText( trUtf8( "Name" ) );

    curveComboBox = new QComboBox( FALSE, plotGroupBox, "curveComboBox" );
    curveComboBox->insertItem( image0, trUtf8( "Lines" ) );
    curveComboBox->insertItem( image1, trUtf8( "Sticks" ) );
    curveComboBox->insertItem( image2, trUtf8( "Steps" ) );
    curveComboBox->insertItem( image3, trUtf8( "Dots" ) );
    curveComboBox->insertItem( image4, trUtf8( "Spline" ) );
    curveComboBox->setGeometry( QRect( 70, 20, 110, 23 ) );

    NameEdit = new QLineEdit( plotGroupBox, "NameEdit" );
    NameEdit->setGeometry( QRect( 70, 50, 110, 23 ) );

    GroupBox4 = new QGroupBox( this, "GroupBox4" );
    GroupBox4->setGeometry( QRect( 0, 80, 200, 90 ) );
    GroupBox4->setTitle( trUtf8( "Dots options" ) );

    plot_Style = new QLabel( GroupBox4, "plot_Style" );
    plot_Style->setGeometry( QRect( 10, 20, 52, 20 ) );
    QFont plot_Style_font(  plot_Style->font() );
    plot_Style_font.setBold( TRUE );
    plot_Style->setFont( plot_Style_font );
    plot_Style->setText( trUtf8( "Symbol" ) );

    dotsColorButton = new QPushButton( GroupBox4, "dotsColorButton" );
    dotsColorButton->setGeometry( QRect( 10, 50, 80, 30 ) );
    dotsColorButton->setText( trUtf8( "Color" ) );
    dotsColorButton->setAutoDefault( TRUE );

    plotComboBox = new QComboBox( FALSE, GroupBox4, "plotComboBox" );
    plotComboBox->insertItem( trUtf8( "None" ) );
    plotComboBox->insertItem( image5, trUtf8( "Ellipse" ) );
    plotComboBox->insertItem( image6, trUtf8( "Cross" ) );
    plotComboBox->insertItem( image7, trUtf8( "Diamond" ) );
    plotComboBox->insertItem( image8, trUtf8( "Rectangle" ) );
    plotComboBox->insertItem( image9, trUtf8( "Triangle" ) );
    plotComboBox->setGeometry( QRect( 70, 20, 110, 23 ) );

    penGroupBox = new QGroupBox( this, "penGroupBox" );
    penGroupBox->setGeometry( QRect( 200, 0, 210, 120 ) );
    penGroupBox->setTitle( trUtf8( "Pen options" ) );

    pen_Style = new QLabel( penGroupBox, "pen_Style" );
    pen_Style->setGeometry( QRect( 10, 20, 35, 20 ) );
    QFont pen_Style_font(  pen_Style->font() );
    pen_Style_font.setBold( TRUE );
    pen_Style->setFont( pen_Style_font );
    pen_Style->setText( trUtf8( "Style" ) );

    Width = new QLabel( penGroupBox, "Width" );
    Width->setGeometry( QRect( 10, 50, 40, 20 ) );
    QFont Width_font(  Width->font() );
    Width_font.setBold( TRUE );
    Width->setFont( Width_font );
    Width->setText( trUtf8( "Width" ) );

    penComboBox = new QComboBox( FALSE, penGroupBox, "penComboBox" );
    penComboBox->insertItem( image10, trUtf8( "Solid line" ) );
    penComboBox->insertItem( image11, trUtf8( "Dash line" ) );
    penComboBox->insertItem( image12, trUtf8( "Dot line" ) );
    penComboBox->insertItem( image13, trUtf8( "Dash dot line" ) );
    penComboBox->insertItem( image14, trUtf8( "Dash dot dot line" ) );
    penComboBox->setGeometry( QRect( 50, 20, 140, 23 ) );

    ColorButton = new QPushButton( penGroupBox, "ColorButton" );
    ColorButton->setGeometry( QRect( 10, 80, 80, 30 ) );
    ColorButton->setText( trUtf8( "&Color" ) );
    ColorButton->setAutoDefault( TRUE );

    widthSpinBox = new QSpinBox( penGroupBox, "widthSpinBox" );
    widthSpinBox->setGeometry( QRect( 50, 50, 140, 23 ) );

    validGroupBox = new QGroupBox( this, "validGroupBox" );
    validGroupBox->setGeometry( QRect( 200, 120, 210, 50 ) );
    validGroupBox->setTitle( trUtf8( "" ) );

    CancelButton = new QPushButton( validGroupBox, "CancelButton" );
    CancelButton->setGeometry( QRect( 110, 10, 91, 32 ) );
    CancelButton->setText( trUtf8( "&Cancel" ) );
    CancelButton->setAutoDefault( TRUE );

    OKButton = new QPushButton( validGroupBox, "OKButton" );
    OKButton->setGeometry( QRect( 10, 10, 91, 32 ) );
    OKButton->setText( trUtf8( "&OK" ) );
    OKButton->setDefault( TRUE );

    // signals and slots connections
    connect( curveComboBox, SIGNAL( activated(int) ), this, SLOT( set_CurveStyle(int) ) );
    connect( penComboBox, SIGNAL( activated(int) ), this, SLOT( set_PenStyle(int) ) );
    connect( plotComboBox, SIGNAL( activated(int) ), this, SLOT( drawMarker(int) ) );
    
    connect( ColorButton, SIGNAL( clicked() ), this, SLOT( setColor() ) );
    connect( OKButton, SIGNAL( clicked() ), this, SLOT( validProperties() ) );
    connect( CancelButton, SIGNAL( clicked() ), this, SLOT( close() ) );
    connect( dotsColorButton, SIGNAL( clicked() ), this, SLOT( setDotsColor() ) );

    connect( NameEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setCurveName() ) );
    connect( widthSpinBox, SIGNAL( valueChanged(int) ), this, SLOT( setWidth() ) );
}

/*
 *  Destroys the object and frees any allocated resources
 */
PropertiesForm::~PropertiesForm()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

