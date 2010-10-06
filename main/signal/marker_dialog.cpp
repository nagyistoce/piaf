/****************************************************************************
** Form implementation generated from reading ui file 'marker.ui'
**
** Created: lun jan 20 12:14:22 2003
**      by:  The User Interface Compiler (uic)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/
#include "marker_dialog.h"

#include <qvariant.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qlistbox.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qimage.h>
#include <qpixmap.h>

#include "marker_dialog.ui.h"
static const char* const image0_data[] = {
"10 10 3 1",
". c None",
"# c #000000",
"a c #a0a0a4",
"..........",
"..........",
"..####....",
".##aa##...",
".#aaaa#...",
".#aaaa#...",
".##aa##...",
".#####....",
"..........",
".........."};

static const char* const image1_data[] = {
"10 10 2 1",
". c None",
"# c #000000",
"..........",
"..........",
".#....#...",
"..#..#....",
"...##.....",
"...##.....",
"..#..#....",
".#....#...",
"..........",
".........."};

static const char* const image2_data[] = {
"10 10 3 1",
". c None",
"# c #000000",
"a c #a0a0a4",
"..........",
"..........",
"....#.....",
"...#a#....",
"..#aa#....",
".#aaaa#...",
"..##a#....",
"....#.....",
"..........",
".........."};

static const char* const image3_data[] = {
"10 10 3 1",
". c None",
"# c #000000",
"a c #a0a0a4",
"..........",
"..........",
".######...",
".#aaaa#...",
".#aaaa#...",
".#aaaa#...",
".#aaaa#...",
".######...",
"..........",
".........."};

static const char* const image4_data[] = {
"10 10 3 1",
". c None",
"# c #000000",
"a c #a0a0a4",
"..........",
"..........",
"....#.....",
"...##.....",
"...#a#....",
"..#aa#....",
"..#aaa#...",
".######...",
"..........",
".........."};

static const char* const image5_data[] = {
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

static const char* const image6_data[] = {
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

static const char* const image7_data[] = {
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

static const char* const image8_data[] = {
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

static const char* const image9_data[] = {
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
 *  Constructs a MarkerDialogForm which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
MarkerDialogForm::MarkerDialogForm( QWidget* parent,  const char* name, bool modal, WFlags fl )
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
    if ( !name )
        setName( "MarkerDialogForm" );
    setFixedSize( 449, 181 );
    setCaption( trUtf8( "Marker dialog" ) );

    GroupBox2 = new QGroupBox( this, "GroupBox2" );
    GroupBox2->setGeometry( QRect( 150, 90, 100, 90 ) );
    GroupBox2->setTitle( trUtf8( "Position" ) );

    xposTextLabel = new QLabel( GroupBox2, "xposTextLabel" );
    xposTextLabel->setGeometry( QRect( 10, 20, 16, 20 ) );
    QFont xposTextLabel_font(  xposTextLabel->font() );
    xposTextLabel_font.setPointSize( 12 );
    xposTextLabel->setFont( xposTextLabel_font );
    xposTextLabel->setText( trUtf8( "x" ) );

    yposTextLabel = new QLabel( GroupBox2, "yposTextLabel" );
    yposTextLabel->setGeometry( QRect( 10, 50, 16, 20 ) );
    QFont yposTextLabel_font(  yposTextLabel->font() );
    yposTextLabel_font.setPointSize( 12 );
    yposTextLabel->setFont( yposTextLabel_font );
    yposTextLabel->setText( trUtf8( "y" ) );

    xposEdit = new QLineEdit( GroupBox2, "xposEdit" );
    xposEdit->setGeometry( QRect( 30, 20, 60, 23 ) );

    yposEdit = new QLineEdit( GroupBox2, "yposEdit" );
    yposEdit->setGeometry( QRect( 30, 50, 60, 23 ) );

    deletePushButton = new QPushButton( this, "deletePushButton" );
    deletePushButton->setGeometry( QRect( 260, 80, 80, 32 ) );
    deletePushButton->setText( trUtf8( "&Delete" ) );

    applyPushButton = new QPushButton( this, "applyPushButton" );
    applyPushButton->setGeometry( QRect( 260, 150, 91, 32 ) );
    applyPushButton->setText( trUtf8( "&Apply" ) );
    applyPushButton->setDefault( TRUE );

    newPushButton = new QPushButton( this, "newPushButton" );
    newPushButton->setGeometry( QRect( 260, 30, 80, 32 ) );
    newPushButton->setText( trUtf8( "&New" ) );

    closePushButton = new QPushButton( this, "closePushButton" );
    closePushButton->setGeometry( QRect( 360, 150, 91, 32 ) );
    closePushButton->setText( trUtf8( "&Close" ) );

    ListBox = new QListBox( this, "ListBox" );
    ListBox->setGeometry( QRect( 350, 10, 96, 130 ) );

    GroupBox5 = new QGroupBox( this, "GroupBox5" );
    GroupBox5->setGeometry( QRect( 150, 0, 100, 90 ) );
    GroupBox5->setTitle( trUtf8( "Symbol" ) );

    symbolComboBox = new QComboBox( FALSE, GroupBox5, "symbolComboBox" );
    symbolComboBox->insertItem( trUtf8( "None" ) );
    symbolComboBox->insertItem( image0, trUtf8( "Ellipse" ) );
    symbolComboBox->insertItem( image1, trUtf8( "Cross" ) );
    symbolComboBox->insertItem( image2, trUtf8( "Diamond" ) );
    symbolComboBox->insertItem( image3, trUtf8( "Rectangle" ) );
    symbolComboBox->insertItem( image4, trUtf8( "Triangle" ) );
    symbolComboBox->setGeometry( QRect( 10, 20, 80, 23 ) );

    PushButton6 = new QPushButton( GroupBox5, "PushButton6" );
    PushButton6->setGeometry( QRect( 50, 50, 40, 32 ) );
    PushButton6->setText( trUtf8( "" ) );

    mrkColorLabel_2 = new QLabel( GroupBox5, "mrkColorLabel_2" );
    mrkColorLabel_2->setGeometry( QRect( 10, 50, 43, 20 ) );
    QFont mrkColorLabel_2_font(  mrkColorLabel_2->font() );
    mrkColorLabel_2_font.setPointSize( 12 );
    mrkColorLabel_2->setFont( mrkColorLabel_2_font );
    mrkColorLabel_2->setText( trUtf8( "Color" ) );

    GroupBox1 = new QGroupBox( this, "GroupBox1" );
    GroupBox1->setGeometry( QRect( 0, 0, 150, 180 ) );
    GroupBox1->setTitle( trUtf8( "marker Properties" ) );

    TextLabel5 = new QLabel( GroupBox1, "TextLabel5" );
    TextLabel5->setGeometry( QRect( 10, 80, 40, 20 ) );
    QFont TextLabel5_font(  TextLabel5->font() );
    TextLabel5_font.setPointSize( 12 );
    TextLabel5->setFont( TextLabel5_font );
    TextLabel5->setText( trUtf8( "Style" ) );

    modeTextLabel = new QLabel( GroupBox1, "modeTextLabel" );
    modeTextLabel->setGeometry( QRect( 10, 50, 40, 20 ) );
    QFont modeTextLabel_font(  modeTextLabel->font() );
    modeTextLabel_font.setPointSize( 12 );
    modeTextLabel->setFont( modeTextLabel_font );
    modeTextLabel->setText( trUtf8( "Mode" ) );

    NameEdit = new QLineEdit( GroupBox1, "NameEdit" );
    NameEdit->setGeometry( QRect( 60, 20, 80, 23 ) );
    NameEdit->setText( trUtf8( "" ) );

    nameLabel = new QLabel( GroupBox1, "nameLabel" );
    nameLabel->setGeometry( QRect( 10, 20, 38, 20 ) );
    QFont nameLabel_font(  nameLabel->font() );
    nameLabel_font.setBold( TRUE );
    nameLabel->setFont( nameLabel_font );
    nameLabel->setText( trUtf8( "Label" ) );

    widthSpinBox = new QSpinBox( GroupBox1, "widthSpinBox" );
    widthSpinBox->setGeometry( QRect( 60, 110, 80, 23 ) );

    styleComboBox = new QComboBox( FALSE, GroupBox1, "styleComboBox" );
    styleComboBox->insertItem( image5, trUtf8( "" ) );
    styleComboBox->insertItem( image6, trUtf8( "" ) );
    styleComboBox->insertItem( image7, trUtf8( "" ) );
    styleComboBox->insertItem( image8, trUtf8( "" ) );
    styleComboBox->insertItem( image9, trUtf8( "" ) );
    styleComboBox->setGeometry( QRect( 60, 80, 80, 23 ) );

    modeComboBox = new QComboBox( FALSE, GroupBox1, "modeComboBox" );
    modeComboBox->insertItem( trUtf8( "No line" ) );
    modeComboBox->insertItem( trUtf8( "Horizontal" ) );
    modeComboBox->insertItem( trUtf8( "Vertical" ) );
    modeComboBox->insertItem( trUtf8( "Cross" ) );
    modeComboBox->setGeometry( QRect( 60, 50, 80, 23 ) );

    widthLabel = new QLabel( GroupBox1, "widthLabel" );
    widthLabel->setGeometry( QRect( 10, 110, 45, 20 ) );
    QFont widthLabel_font(  widthLabel->font() );
    widthLabel_font.setPointSize( 12 );
    widthLabel->setFont( widthLabel_font );
    widthLabel->setText( trUtf8( "Width" ) );

    mrkColorLabel = new QLabel( GroupBox1, "mrkColorLabel" );
    mrkColorLabel->setGeometry( QRect( 10, 140, 45, 20 ) );
    QFont mrkColorLabel_font(  mrkColorLabel->font() );
    mrkColorLabel_font.setPointSize( 12 );
    mrkColorLabel->setFont( mrkColorLabel_font );
    mrkColorLabel->setText( trUtf8( "Color" ) );

    colorPushButton = new QPushButton( GroupBox1, "colorPushButton" );
    colorPushButton->setGeometry( QRect( 60, 140, 40, 32 ) );
    colorPushButton->setText( trUtf8( "" ) );

    // signals and slots connections
    connect( xposEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setMarkerXPos() ) );
    connect( yposEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setMarkerYPos() ) );
    connect( NameEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setMarkerLabel() ) );

    connect( modeComboBox, SIGNAL( highlighted(int) ), this, SLOT( setMarkerMode(int) ) );
    connect( styleComboBox, SIGNAL( highlighted(int) ), this, SLOT( setMarkerStyle(int) ) );
    connect( symbolComboBox, SIGNAL( highlighted(int) ), this, SLOT( setMarkerSymbol(int) ) );
    
    connect( colorPushButton, SIGNAL( clicked() ), this, SLOT( setMarkerColor() ) );
    connect( PushButton6, SIGNAL( clicked() ), this, SLOT( setSymbolColor() ) );
    connect( deletePushButton, SIGNAL( clicked() ), this, SLOT( removeMarker() ) );
    connect( newPushButton, SIGNAL( clicked() ), this, SLOT( newMarker() ) );
    connect( applyPushButton, SIGNAL( clicked() ), this, SLOT( applyProperties() ) );
    connect( closePushButton, SIGNAL( clicked() ), this, SLOT( closeMarkerDialog() ) );

    connect( widthSpinBox, SIGNAL( valueChanged(int) ), this, SLOT( setMarkerWidth() ) );
    connect( ListBox, SIGNAL( selectionChanged(QListBoxItem*) ), this, SLOT( itemSelected(QListBoxItem*) ) );
}

/*
 *  Destroys the object and frees any allocated resources
 */
MarkerDialogForm::~MarkerDialogForm()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

