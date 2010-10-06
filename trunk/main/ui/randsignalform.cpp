/****************************************************************************
** Form implementation generated from reading ui file 'ui/randsignalform.ui'
**
** Created: Wed Feb 24 17:22:27 2010
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#include "randsignalform.h"

#include <qvariant.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qimage.h>
#include <qpixmap.h>

#include "math.h"
#include "randsignalform.ui.h"
/*
 *  Constructs a RandSignalForm as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
RandSignalForm::RandSignalForm( QWidget* parent, const char* name, bool modal, WFlags fl )
    : QDialog( parent, name, modal, fl )
{
    if ( !name )
	setName( "RandSignalForm" );
    setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)0, (QSizePolicy::SizeType)0, 0, 0, sizePolicy().hasHeightForWidth() ) );
    setMinimumSize( QSize( 600, 500 ) );

    textLabel1 = new QLabel( this, "textLabel1" );
    textLabel1->setGeometry( QRect( 70, 110, 80, 20 ) );
    QFont textLabel1_font(  textLabel1->font() );
    textLabel1_font.setPointSize( 14 );
    textLabel1->setFont( textLabel1_font ); 
    textLabel1->setFrameShape( QLabel::NoFrame );
    textLabel1->setFrameShadow( QLabel::Plain );

    textLabel1_3 = new QLabel( this, "textLabel1_3" );
    textLabel1_3->setGeometry( QRect( 70, 60, 80, 20 ) );
    QFont textLabel1_3_font(  textLabel1_3->font() );
    textLabel1_3_font.setPointSize( 14 );
    textLabel1_3->setFont( textLabel1_3_font ); 

    textLabel1_2 = new QLabel( this, "textLabel1_2" );
    textLabel1_2->setGeometry( QRect( 70, 10, 80, 20 ) );
    QFont textLabel1_2_font(  textLabel1_2->font() );
    textLabel1_2_font.setPointSize( 14 );
    textLabel1_2->setFont( textLabel1_2_font ); 

    paramEdit2 = new QLineEdit( this, "paramEdit2" );
    paramEdit2->setGeometry( QRect( 460, 60, 120, 22 ) );
    paramEdit2->setFrameShadow( QLineEdit::Sunken );

    paramEdit3 = new QLineEdit( this, "paramEdit3" );
    paramEdit3->setGeometry( QRect( 460, 110, 120, 22 ) );
    paramEdit3->setFrameShadow( QLineEdit::Sunken );

    paramLabel3 = new QLabel( this, "paramLabel3" );
    paramLabel3->setGeometry( QRect( 400, 110, 40, 20 ) );
    QFont paramLabel3_font(  paramLabel3->font() );
    paramLabel3_font.setPointSize( 14 );
    paramLabel3->setFont( paramLabel3_font ); 

    paramLabel2 = new QLabel( this, "paramLabel2" );
    paramLabel2->setGeometry( QRect( 400, 60, 40, 20 ) );
    QFont paramLabel2_font(  paramLabel2->font() );
    paramLabel2_font.setPointSize( 14 );
    paramLabel2->setFont( paramLabel2_font ); 

    paramLabel1 = new QLabel( this, "paramLabel1" );
    paramLabel1->setGeometry( QRect( 400, 10, 50, 20 ) );
    QFont paramLabel1_font(  paramLabel1->font() );
    paramLabel1_font.setPointSize( 14 );
    paramLabel1->setFont( paramLabel1_font ); 
    paramLabel1->setTextFormat( QLabel::PlainText );

    Close_Button = new QPushButton( this, "Close_Button" );
    Close_Button->setGeometry( QRect( 370, 170, 91, 31 ) );

    samplesEdit = new QLineEdit( this, "samplesEdit" );
    samplesEdit->setGeometry( QRect( 200, 110, 131, 22 ) );
    samplesEdit->setFrameShadow( QLineEdit::Sunken );

    signalTypeCombo = new QComboBox( FALSE, this, "signalTypeCombo" );
    signalTypeCombo->setGeometry( QRect( 200, 10, 131, 21 ) );

    paramEdit1 = new QLineEdit( this, "paramEdit1" );
    paramEdit1->setGeometry( QRect( 460, 10, 120, 22 ) );
    paramEdit1->setFrameShadow( QLineEdit::Sunken );

    nameEdit = new QLineEdit( this, "nameEdit" );
    nameEdit->setGeometry( QRect( 200, 60, 131, 22 ) );
    nameEdit->setFrameShadow( QLineEdit::Sunken );

    CreateButton = new QPushButton( this, "CreateButton" );
    CreateButton->setGeometry( QRect( 260, 170, 91, 31 ) );

    TestButton = new QPushButton( this, "TestButton" );
    TestButton->setGeometry( QRect( 150, 170, 91, 31 ) );
    TestButton->setToggleButton( FALSE );
    TestButton->setFlat( FALSE );
    languageChange();
    resize( QSize(600, 500).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );

    // signals and slots connections
    connect( Close_Button, SIGNAL( clicked() ), this, SLOT( Close_Button_clicked() ) );
    connect( signalTypeCombo, SIGNAL( activated(int) ), this, SLOT( signalTypeCombo_textChanged(int) ) );
    connect( TestButton, SIGNAL( released() ), this, SLOT( TestButton_released() ) );
    connect( CreateButton, SIGNAL( released() ), this, SLOT( CreateButton_released() ) );
    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
RandSignalForm::~RandSignalForm()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void RandSignalForm::languageChange()
{
    setCaption( tr( "Random Signal generation" ) );
    textLabel1->setText( tr( "Samples" ) );
    textLabel1_3->setText( tr( "Name" ) );
    textLabel1_2->setText( tr( "Type" ) );
    paramLabel3->setText( tr( "Phi" ) );
    paramLabel2->setText( tr( "w" ) );
    paramLabel1->setText( tr( "Gain" ) );
    Close_Button->setText( tr( "Close", "Click to close this window" ) );
    signalTypeCombo->clear();
    signalTypeCombo->insertItem( tr( "Sinus" ) );
    signalTypeCombo->insertItem( tr( "Square" ) );
    signalTypeCombo->insertItem( tr( "White noise" ) );
    signalTypeCombo->insertItem( tr( "Random example" ) );
    CreateButton->setText( tr( "Create", "Click to create a new measure" ) );
    TestButton->setText( tr( "Test !", "Click to see the generated signal" ) );
}

