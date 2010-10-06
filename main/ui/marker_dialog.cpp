/****************************************************************************
** Form implementation generated from reading ui file 'ui/marker_dialog.ui'
**
** Created: Wed Feb 24 17:22:28 2010
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#include "marker_dialog.h"

#include <qvariant.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qlistbox.h>
#include <qcombobox.h>
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
 *  Constructs a MarkerDialogForm as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
MarkerDialogForm::MarkerDialogForm( QWidget* parent, const char* name, bool modal, WFlags fl )
    : QDialog( parent, name, modal, fl ),
      image0( (const char **) image0_data ),
      image1( (const char **) image1_data ),
      image2( (const char **) image2_data ),
      image3( (const char **) image3_data ),
      image4( (const char **) image4_data ),
      image5( (const char **) image5_data ),
      image6( (const char **) image6_data ),
      image7( (const char **) image7_data ),
      image8( (const char **) image8_data ),
      image9( (const char **) image9_data )
{
    if ( !name )
	setName( "MarkerDialogForm" );

    GroupBox2 = new QGroupBox( this, "GroupBox2" );
    GroupBox2->setGeometry( QRect( 150, 90, 100, 90 ) );
    GroupBox2->setFrameShape( QGroupBox::Box );
    GroupBox2->setFrameShadow( QGroupBox::Sunken );

    xposTextLabel = new QLabel( GroupBox2, "xposTextLabel" );
    xposTextLabel->setGeometry( QRect( 10, 20, 16, 20 ) );
    QFont xposTextLabel_font(  xposTextLabel->font() );
    xposTextLabel->setFont( xposTextLabel_font ); 

    yposTextLabel = new QLabel( GroupBox2, "yposTextLabel" );
    yposTextLabel->setGeometry( QRect( 10, 50, 16, 20 ) );
    QFont yposTextLabel_font(  yposTextLabel->font() );
    yposTextLabel->setFont( yposTextLabel_font ); 

    xposEdit = new QLineEdit( GroupBox2, "xposEdit" );
    xposEdit->setGeometry( QRect( 30, 20, 60, 23 ) );

    yposEdit = new QLineEdit( GroupBox2, "yposEdit" );
    yposEdit->setGeometry( QRect( 30, 50, 60, 23 ) );

    ListBox = new QListBox( this, "ListBox" );
    ListBox->setGeometry( QRect( 350, 10, 96, 130 ) );

    GroupBox5 = new QGroupBox( this, "GroupBox5" );
    GroupBox5->setGeometry( QRect( 150, 0, 100, 90 ) );

    symbolComboBox = new QComboBox( FALSE, GroupBox5, "symbolComboBox" );
    symbolComboBox->setGeometry( QRect( 10, 20, 80, 23 ) );

    PushButton6 = new QPushButton( GroupBox5, "PushButton6" );
    PushButton6->setGeometry( QRect( 50, 50, 40, 32 ) );

    mrkColorLabel_2 = new QLabel( GroupBox5, "mrkColorLabel_2" );
    mrkColorLabel_2->setGeometry( QRect( 10, 50, 43, 20 ) );
    QFont mrkColorLabel_2_font(  mrkColorLabel_2->font() );
    mrkColorLabel_2->setFont( mrkColorLabel_2_font ); 

    newPushButton = new QPushButton( this, "newPushButton" );
    newPushButton->setGeometry( QRect( 260, 30, 80, 32 ) );

    deletePushButton = new QPushButton( this, "deletePushButton" );
    deletePushButton->setGeometry( QRect( 260, 80, 80, 32 ) );

    cancelPushButton = new QPushButton( this, "cancelPushButton" );
    cancelPushButton->setGeometry( QRect( 360, 150, 91, 32 ) );

    GroupBox1 = new QGroupBox( this, "GroupBox1" );
    GroupBox1->setGeometry( QRect( 0, 0, 150, 180 ) );

    TextLabel5 = new QLabel( GroupBox1, "TextLabel5" );
    TextLabel5->setGeometry( QRect( 10, 80, 40, 20 ) );
    QFont TextLabel5_font(  TextLabel5->font() );
    TextLabel5->setFont( TextLabel5_font ); 

    modeTextLabel = new QLabel( GroupBox1, "modeTextLabel" );
    modeTextLabel->setGeometry( QRect( 10, 50, 40, 20 ) );
    QFont modeTextLabel_font(  modeTextLabel->font() );
    modeTextLabel->setFont( modeTextLabel_font ); 

    NameEdit = new QLineEdit( GroupBox1, "NameEdit" );
    NameEdit->setGeometry( QRect( 60, 20, 80, 23 ) );

    nameLabel = new QLabel( GroupBox1, "nameLabel" );
    nameLabel->setGeometry( QRect( 10, 20, 38, 20 ) );
    QFont nameLabel_font(  nameLabel->font() );
    nameLabel_font.setBold( TRUE );
    nameLabel->setFont( nameLabel_font ); 

    widthSpinBox = new QSpinBox( GroupBox1, "widthSpinBox" );
    widthSpinBox->setGeometry( QRect( 60, 110, 80, 23 ) );

    styleComboBox = new QComboBox( FALSE, GroupBox1, "styleComboBox" );
    styleComboBox->setGeometry( QRect( 60, 80, 80, 23 ) );

    modeComboBox = new QComboBox( FALSE, GroupBox1, "modeComboBox" );
    modeComboBox->setGeometry( QRect( 60, 50, 80, 23 ) );

    widthLabel = new QLabel( GroupBox1, "widthLabel" );
    widthLabel->setGeometry( QRect( 10, 110, 45, 20 ) );
    QFont widthLabel_font(  widthLabel->font() );
    widthLabel->setFont( widthLabel_font ); 

    mrkColorLabel = new QLabel( GroupBox1, "mrkColorLabel" );
    mrkColorLabel->setGeometry( QRect( 10, 140, 45, 20 ) );
    QFont mrkColorLabel_font(  mrkColorLabel->font() );
    mrkColorLabel->setFont( mrkColorLabel_font ); 

    colorPushButton = new QPushButton( GroupBox1, "colorPushButton" );
    colorPushButton->setGeometry( QRect( 60, 140, 40, 32 ) );

    applyPushButton = new QPushButton( this, "applyPushButton" );
    applyPushButton->setGeometry( QRect( 260, 150, 80, 32 ) );
    applyPushButton->setDefault( TRUE );
    languageChange();
    resize( QSize(449, 181).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );

    // signals and slots connections
    connect( xposEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setMarkerXPos() ) );
    connect( yposEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setMarkerYPos() ) );
    connect( NameEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setMarkerLabel() ) );
    connect( widthSpinBox, SIGNAL( valueChanged(int) ), this, SLOT( setMarkerWidth() ) );
    connect( colorPushButton, SIGNAL( clicked() ), this, SLOT( setMarkerColor() ) );
    connect( PushButton6, SIGNAL( clicked() ), this, SLOT( setSymbolColor() ) );
    connect( deletePushButton, SIGNAL( clicked() ), this, SLOT( removeMarker() ) );
    connect( newPushButton, SIGNAL( clicked() ), this, SLOT( newMarker() ) );
    connect( applyPushButton, SIGNAL( clicked() ), this, SLOT( applyProperties() ) );
    connect( ListBox, SIGNAL( selectionChanged(QListBoxItem*) ), this, SLOT( itemSelected(QListBoxItem*) ) );
    connect( cancelPushButton, SIGNAL( clicked() ), this, SLOT( closeMarkerDialog() ) );
    connect( symbolComboBox, SIGNAL( highlighted(int) ), this, SLOT( setMarkerSymbol(int) ) );
    connect( modeComboBox, SIGNAL( highlighted(int) ), this, SLOT( setMarkerMode(int) ) );
    connect( styleComboBox, SIGNAL( highlighted(int) ), this, SLOT( setMarkerStyle(int) ) );
}

/*
 *  Destroys the object and frees any allocated resources
 */
MarkerDialogForm::~MarkerDialogForm()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void MarkerDialogForm::languageChange()
{
    setCaption( tr( "Marker dialog" ) );
    GroupBox2->setTitle( tr( "Position" ) );
    xposTextLabel->setText( tr( "x" ) );
    yposTextLabel->setText( tr( "y" ) );
    GroupBox5->setTitle( tr( "Symbol" ) );
    symbolComboBox->clear();
    symbolComboBox->insertItem( tr( "None" ) );
    symbolComboBox->insertItem( image0, tr( "Ellipse" ) );
    symbolComboBox->insertItem( image1, tr( "Cross" ) );
    symbolComboBox->insertItem( image2, tr( "Diamond" ) );
    symbolComboBox->insertItem( image3, tr( "Rectangle" ) );
    symbolComboBox->insertItem( image4, tr( "Triangle" ) );
    PushButton6->setText( QString::null );
    mrkColorLabel_2->setText( tr( "Color" ) );
    newPushButton->setText( tr( "&New" ) );
    deletePushButton->setText( tr( "&Delete" ) );
    cancelPushButton->setText( tr( "&Close" ) );
    GroupBox1->setTitle( tr( "marker Properties" ) );
    TextLabel5->setText( tr( "Style" ) );
    modeTextLabel->setText( tr( "Mode" ) );
    NameEdit->setText( QString::null );
    nameLabel->setText( tr( "Label" ) );
    styleComboBox->clear();
    styleComboBox->insertItem( image5, QString::null );
    styleComboBox->insertItem( image6, QString::null );
    styleComboBox->insertItem( image7, QString::null );
    styleComboBox->insertItem( image8, QString::null );
    styleComboBox->insertItem( image9, QString::null );
    modeComboBox->clear();
    modeComboBox->insertItem( tr( "No line" ) );
    modeComboBox->insertItem( tr( "Horizontal" ) );
    modeComboBox->insertItem( tr( "Vertical" ) );
    modeComboBox->insertItem( tr( "Cross" ) );
    widthLabel->setText( tr( "Width" ) );
    mrkColorLabel->setText( tr( "Color" ) );
    colorPushButton->setText( QString::null );
    applyPushButton->setText( tr( "&Apply" ) );
}

